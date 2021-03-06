#include "glfuncs.h"

#include "utils/util.h"
#include "texture-formats/tgareader.h"
#include "texture-formats/pngreader.h"
#include "texture-formats/bmpreader.h"

#include <iostream>
#include <boost\filesystem.hpp>


//Crea un buffer per OpenGL
GLuint make_buffer(
	GLenum target,
	const void *buffer_data,
	GLsizei buffer_size
	)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(target, buffer);
	glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
	return buffer;
}

void *read_texture(const char *filename, int *width, int *height, GLuint &format)
{
	std::string extension = boost::filesystem::extension(filename);

	if (extension == ".png")
	{
		format = GL_RGBA;
		return read_png(filename, (unsigned *) width, (unsigned *) height);
	}

	if (extension == ".tga")
	{
		format = GL_BGR;
		return read_tga(filename, width, height);
	}

	if (extension == ".bmp")
	{
		format = GL_BGR;
		return read_bmp(filename, width, height);
	}

	return NULL;
}

//Crea una texure per OpenGL
GLuint make_texture(const char *filename)
{
	GLuint texture;
	int width, height;
	GLuint format;
	void *pixels = read_texture(filename, &width, &height, format);


	if (!pixels)
		return 0;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

	glTexImage2D(
		GL_TEXTURE_2D, 0,           /* target, level of detail */
		GL_RGB8,                    /* internal format */
		width, height, 0,           /* width, height, border */
		format, GL_UNSIGNED_BYTE,   /* external format, type */
		pixels                      /* pixels */
		);
	free(pixels);
	return texture;
}
void show_info_log(
	GLuint object,
	PFNGLGETSHADERIVPROC glGet__iv,
	PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
)
{
	GLint log_length;
	char *log;

	glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
	log = (char*)malloc(log_length);
	glGet__InfoLog(object, log_length, NULL, log);
	fprintf(stderr, "%s", log);
	free(log);
}

GLuint make_shader(GLenum type, const char *filename)
{
	GLint length;
	GLchar *source = (char*)file_contents(filename, &length);
	GLuint shader;
	GLint shader_ok;

	if (!source)
		return 0;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	free(source);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok) {
		fprintf(stderr, "Failed to compile %s:\n", filename);
		show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
	GLint program_ok;

	GLuint program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
	if (!program_ok) {
		fprintf(stderr, "Failed to link shader program:\n");
		show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
		glDeleteProgram(program);
		return 0;
	}
	return program;
}