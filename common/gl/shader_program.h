
#include <Glee.h>

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace gl {
		
class shader_program : boost::noncopyable
{
public:
	shader_program() : program_(0){}
	shader_program(shader_program&& other) : program_(other.program_){}
	shader_program& operator=(shader_program&& other);

	shader_program(const std::string& vertex_source_str, const std::string& fragment_source_str);
	~shader_program();

	void use();

	GLuint program() { return program_; }
	
private:
	GLuint program_;
};
typedef std::shared_ptr<shader_program> shader_program_ptr;

}}