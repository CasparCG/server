#include "qtquick_producer.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/future.h>
#include <common/log.h>
#include <common/os/filesystem.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace caspar { namespace qtquick {

class qt_renderer
{
    using loaded_callback_t = std::function<void()>;

    const loaded_callback_t                    loaded_callback_;
    const core::video_format_desc              format_desc_;
    const spl::shared_ptr<core::frame_factory> frame_factory_;

    QQuickRenderControl                       control_;
    QOpenGLContext                            context_;
    QOffscreenSurface                         surface_;
    std::unique_ptr<QOpenGLFramebufferObject> fbo_;
    QQmlEngine                                engine_;
    QQuickWindow                              window_;
    std::unique_ptr<QQmlComponent>            qmlComponent_;

    spl::shared_ptr<diagnostics::graph> graph_;

    QTimer                      updateTimer_;
    std::unique_ptr<QQuickItem> rootItem_;

    mutable std::mutex frame_mutex_;
    core::draw_frame   frame_;

    void createFbo()
    {
        fbo_ = std::unique_ptr<QOpenGLFramebufferObject>(new QOpenGLFramebufferObject(
            QSize(format_desc_.width, format_desc_.height), QOpenGLFramebufferObject::CombinedDepthStencil));
        window_.setRenderTarget(fbo_.get());
    }

    void destroyFbo() { fbo_.reset(nullptr); }

    void requestUpdate()
    {
        if (!updateTimer_.isActive())
            updateTimer_.start();
    }

    void render()
    {
        boost::timer timer;
        timer.restart();

        if (!context_.makeCurrent(&surface_))
            return;

        // Polish, synchronize and render the next frame (into our fbo).  In this example
        // everything happens on the same thread and therefore all three steps are performed
        // in succession from here. In a threaded setup the render() call would happen on a
        // separate thread.
        control_.polishItems();
        control_.sync();
        control_.render();

        window_.resetOpenGLState();
        QOpenGLFramebufferObject::bindDefault();

        context_.functions()->glFlush();

        QImage image  = fbo_->toImage();
        int    width  = image.width();
        int    height = image.height();

        core::pixel_format_desc pixel_desc;
        pixel_desc.format = core::pixel_format::bgra;
        pixel_desc.planes.push_back(core::pixel_format_desc::plane(width, height, 4));

        auto                 frame  = frame_factory_->create_frame(this, pixel_desc);
        const unsigned char* buffer = image.bits();
        std::memcpy(frame.image_data(0).begin(), buffer, width * height * 4);
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            frame_ = core::draw_frame(std::move(frame));
        }

        graph_->set_value("render-time", timer.elapsed() * format_desc_.fps);
    }

    void run()
    {
        if (qmlComponent_->isError()) {
            const QList<QQmlError> errorList = qmlComponent_->errors();
            for (const QQmlError& error : errorList)
                qWarning() << error.url() << error.line() << error;
            return;
        }

        QObject* rootObject = qmlComponent_->create();
        if (qmlComponent_->isError()) {
            const QList<QQmlError> errorList = qmlComponent_->errors();
            for (const QQmlError& error : errorList)
                qWarning() << error.url() << error.line() << error;
            return;
        }

        rootItem_.reset(qobject_cast<QQuickItem*>(rootObject));
        if (!rootItem_) {
            qWarning("run: Not a QQuickItem");
            delete rootObject;
            return;
        }

        // The root item is ready. Associate it with the window.
        rootItem_->setParentItem(window_.contentItem());

        // Update item and rendering related geometries.
        updateSizes();

        // Initialize the render control and our OpenGL resources.
        context_.makeCurrent(&surface_);
        control_.initialize(&context_);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [this]() { requestUpdate(); });
        loaded_callback_();
    }

    void updateSizes()
    {
        // Behave like SizeRootObjectToView.
        rootItem_->setWidth(format_desc_.width);
        rootItem_->setHeight(format_desc_.height);

        window_.setGeometry(0, 0, format_desc_.width, format_desc_.height);
    }

  public:
    qt_renderer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                const loaded_callback_t&                    loaded_callback,
                core::video_format_desc                     format_desc)
        : loaded_callback_(loaded_callback)
        , format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , window_(&control_)
        , frame_(core::draw_frame{})
    {
        graph_->set_color("render-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_text(L"test");
        diagnostics::register_graph(graph_);

        // All of Qt must be initialized from Qt's main thread to work.
        assert(QThread::currentThread() == QGuiApplication::instance()->thread());

        QSurfaceFormat format;
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);

        context_.setFormat(format);
        context_.create();

        surface_.setFormat(format);
        surface_.create();

        window_.setColor(Qt::transparent);

        if (!engine_.incubationController())
            engine_.setIncubationController(window_.incubationController());

        updateTimer_.setSingleShot(true);
        updateTimer_.setInterval(5);

        QObject::connect(&updateTimer_, &QTimer::timeout, [this]() { render(); });

        QObject::connect(&window_, &QQuickWindow::sceneGraphInitialized, [this]() { createFbo(); });

        QObject::connect(&window_, &QQuickWindow::sceneGraphInvalidated, [this]() { destroyFbo(); });
    }

    ~qt_renderer()
    {
        context_.makeCurrent(&surface_);
        control_.invalidate();
    }

    void startQuick(const QString& filename)
    {
        qmlComponent_ = std::unique_ptr<QQmlComponent>(new QQmlComponent(&engine_, QUrl(filename)));

        if (qmlComponent_->isLoading()) {
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn     = QObject::connect(qmlComponent_.get(), &QQmlComponent::statusChanged, [this, conn]() {
                QObject::disconnect(*conn);
                run();
            });
        } else {
            run();
        }
    }

    void execute_javascript(const std::vector<std::wstring>& params)
    {
        assert(rootItem_);
        if (params.size() == 1)
            QMetaObject::invokeMethod(rootItem_.get(), u8(params.at(0)).c_str());
        else
            QMetaObject::invokeMethod(
                rootItem_.get(), u8(params.at(0)).c_str(), Q_ARG(QVariant, QString::fromStdWString(params.at(1))));
    }

    core::draw_frame receive_frame()
    {
        core::draw_frame frame;

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            frame = frame_;
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [this]() { requestUpdate(); });

        return frame;
    }
};

class qtquick_producer : public core::frame_producer
{
    const std::wstring url_;

    std::unique_ptr<qt_renderer> renderer_;

    tbb::concurrent_queue<std::vector<std::wstring>> javascript_before_load_;
    tbb::atomic<bool>                                loaded_;

  public:
    qtquick_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                     const core::video_format_desc&              format_desc,
                     const std::wstring&                         url)
        : url_(url)
    {
        loaded_ = false;

        QMetaObject::invokeMethod(QCoreApplication::instance(),
                                  [this, url, frame_factory, format_desc]() {
                                      auto loaded_callback = [this]() { renderer_loaded(); };
                                      renderer_ =
                                          std::make_unique<qt_renderer>(frame_factory, loaded_callback, format_desc);
                                      renderer_->startQuick(QString::fromStdWString(url));
                                  },
                                  Qt::QueuedConnection);
    }

    ~qtquick_producer()
    {
        qt_renderer* renderer = renderer_.release();
        QMetaObject::invokeMethod(
            QCoreApplication::instance(), [renderer]() { delete renderer; }, Qt::QueuedConnection);
    }

    void renderer_loaded()
    {
        loaded_ = true;
        execute_queued_javascript();
    }

    std::wstring name() const override { return L"qtquick"; }

    core::draw_frame receive_impl(int nb_samples) override
    {
        if (renderer_) {
            return renderer_->receive_frame();
        }

        return core::draw_frame{};
    }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        if (!loaded_) {
            javascript_before_load_.push(params);
        } else {
            execute_queued_javascript();
            renderer_->execute_javascript(params);
        }

        return make_ready_future(std::wstring(L""));
    }

    void execute_queued_javascript()
    {
        std::vector<std::wstring> params;

        while (javascript_before_load_.try_pop(params))
            renderer_->execute_javascript(params);
    }

    std::wstring print() const override { return L"qtquick[" + url_ + L"]"; }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    const auto filename       = env::template_folder() + params.at(0) + L".qml";
    const auto found_filename = find_case_insensitive(filename);
    const auto url_prefix     = boost::iequals(params.at(0), L"[URL]");

    if (!found_filename && !url_prefix)
        return core::frame_producer::empty();

    const auto url = found_filename ? QUrl::fromLocalFile(QString::fromStdWString(*found_filename)).toString().toStdWString() : params.at(1);

    if (!url_prefix && (!boost::algorithm::contains(url, ".") || boost::algorithm::ends_with(url, "_A") ||
                        boost::algorithm::ends_with(url, "_ALPHA")))
        return core::frame_producer::empty();

    return core::create_destroy_proxy(
        spl::make_shared<qtquick_producer>(dependencies.frame_factory, dependencies.format_desc, url));
}

}} // namespace caspar::qtquick
