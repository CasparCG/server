#include "qtquick.h"

#include "producer/qtquick_cg_proxy.h"
#include "producer/qtquick_producer.h"

#include <core/producer/cg_proxy.h>

#include <boost/property_tree/ptree.hpp>

#include <common/log.h>

#include <QGuiApplication>
#include <QThread>
#include <QtGlobal>

#include <memory>

// workaround to give QGuiApplication a valid reference for argc
// see https://stackoverflow.com/questions/43825082/qcoreapplication-on-the-heap/43825083#43825083
struct ApplicationHolder
{
    ApplicationHolder(int argc, char* argv[])
    {
        m_argc = new int(argc);
        m_app  = new QGuiApplication(*m_argc, argv);
    }
    ~ApplicationHolder()
    {
        delete m_app;
        delete m_argc;
    }

    int*             m_argc;
    QGuiApplication* m_app;
};

namespace {
// Use a single Qt application event loop for all producers.
std::unique_ptr<ApplicationHolder> application;
std::thread                        thread;

std::mutex              application_started_mutex;
bool                    application_started = false;
std::condition_variable application_started_condition;
} // namespace

namespace caspar { namespace qtquick {

bool intercept_command_line(int argc, char** argv)
{
    thread = std::thread([argc, argv]() {
        application = std::make_unique<ApplicationHolder>(argc, argv);
        QMetaObject::invokeMethod(application->m_app,
                                  []() {
                                      application_started = true;
                                      application_started_condition.notify_all();
                                  },
                                  Qt::QueuedConnection);
        int exit_code = application->m_app->exec();

        CASPAR_LOG(debug) << std::string("Qt application loop exited with code ") << std::to_string(exit_code);
    });

    {
        // Wait for application event loop to be running.
        std::unique_lock<std::mutex> lock(application_started_mutex);
        application_started_condition.wait(lock, []() { return application_started; });
    }

    return false;
}

void init(core::module_dependencies dependencies)
{
    dependencies.producer_registry->register_producer_factory(L"QtQuick Producer", qtquick::create_producer);

    dependencies.cg_registry->register_cg_producer(
        L"qtquick",
        {L".qml"},
        [](const std::wstring& filename) { return ""; },
        [](const spl::shared_ptr<core::frame_producer>& producer) {
            return spl::make_shared<qtquick_cg_proxy>(producer);
        },
        [](const core::frame_producer_dependencies& dependencies, const std::wstring& filename) {
            return qtquick::create_producer(dependencies, {filename});
        },
        false);
}

void uninit()
{
    application->m_app->exit(0);
    application.reset();
}

}} // namespace caspar::qtquick
