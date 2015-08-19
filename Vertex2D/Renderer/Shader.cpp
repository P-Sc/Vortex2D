//
//  Shader.cpp
//  Vortex
//
//  Created by Maximilian Maldacker on 06/04/2014.
//
//

#include "Shader.h"
#include "ResourcePath.h"
#include <fstream>

namespace Renderer
{

const char * Shader::PositionName = "a_Position";
const char * Shader::TexCoordsName = "a_TexCoords";
const char * Shader::ColourName = "a_Colour";

Shader::Shader(GLuint shader) : mShader(shader)
{
}

Shader::~Shader()
{
    if(mShader)
    {
        SDL_Log("Delete shader %d", mShader);
        glDeleteShader(mShader);
    }
}

Shader & Shader::Source(const std::string & source)
{
    std::ifstream sourceFile(getResourcePath() + source);
    std::string sourceFileContent((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());

    auto c_str = sourceFileContent.c_str();
    glShaderSource(mShader, 1, &c_str, NULL);
    return *this;
}

Shader & Shader::Compile()
{
    glCompileShader(mShader);

    GLint status;
    glGetShaderiv(mShader, GL_COMPILE_STATUS, &status);

	if(!status)
    {
		GLsizei length;
		glGetShaderiv(mShader, GL_INFO_LOG_LENGTH, &length);
		GLchar src[length];

		glGetShaderInfoLog(mShader, length, NULL, src);
        SDL_Log("Error compiling shader %s", src);
		throw std::runtime_error("Error compiling shader: " + std::string(src));
	}

    return *this;
}

VertexShader::VertexShader() : Shader(glCreateShader(GL_VERTEX_SHADER))
{
}

FragmentShader::FragmentShader() : Shader(glCreateShader(GL_FRAGMENT_SHADER))
{
}

int Program::CurrentProgram = -1;

Program::Program() : mProgram(glCreateProgram())
{
}

Program::Program(const std::string & vertexSource, const std::string & fragmentSource) : Program()
{
    VertexShader vertex;
    vertex.Source(vertexSource).Compile();

    FragmentShader fragment;
    fragment.Source(fragmentSource).Compile();

    AttachShader(vertex);
    AttachShader(fragment);
    Link();
}

Program::~Program()
{
    if(mProgram)
    {
        SDL_Log("Delete program %d", mProgram);
        glDeleteProgram(mProgram);
    }
}

Program::Program(Program && other)
{
    *this = std::move(other);
}

Program & Program::operator=(Program && other)
{
    mProgram = other.mProgram;
    mMVP.mLocation = other.mMVP.mLocation;

    other.mProgram = 0;
    other.mMVP.mLocation = -1;

    return *this;
}

Program & Program::AttachShader(const Shader &shader)
{
    glAttachShader(mProgram, shader.mShader);
    return *this;
}

Program & Program::Link()
{
	glBindAttribLocation(mProgram, Shader::Position, Shader::PositionName);
    glBindAttribLocation(mProgram, Shader::TexCoords, Shader::TexCoordsName);
    glBindAttribLocation(mProgram, Shader::Colour, Shader::ColourName);

    glLinkProgram(mProgram);

    GLint status;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
    if(!status)
    {
		GLsizei length;
		glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &length);
		GLchar src[length];

		glGetProgramInfoLog(mProgram, length, NULL, src);
        SDL_Log("Error linking program: %s", src);
		throw std::runtime_error("Error linking program: " + std::string(src));
    }

    mMVP.SetLocation(*this, "u_Projection");

    return *this;
}

Program & Program::Use()
{
    if(CurrentProgram != mProgram)
    {
        CurrentProgram = mProgram;
        glUseProgram(mProgram);
    }
    
    return *this;
}

Program & Program::SetMVP(const glm::mat4 &mvp)
{
    mMVP.Set(mvp);
    return *this;
}

Program & Program::TexturePositionProgram()
{
    static Program program;
    if(!program.mProgram)
    {
        program = Program("TexturePosition.vsh", "TexturePosition.fsh");
        program.Use().Set("u_Texture", 0);
    }

    return program;
}

Program & Program::PositionProgram()
{
    static Program program;
    if(!program.mProgram)
    {
        program = Program("Position.vsh", "Position.fsh");
    }

    return program;
}

Program & Program::ColourTexturePositionProgram()
{
    static Program program;
    if(!program.mProgram)
    {
        program = Program("ColourTexturePosition.vsh", "ColourTexturePosition.fsh");
        program.Use().Set("u_Texture", 0);
    }

    return program;
}

Program & Program::ColourPositionProgram()
{
    static Program program;
    if(!program.mProgram)
    {
        program = Program("ColourPosition.vsh", "ColourPosition.fsh");
    }

    return program;
}

Program & Program::CircleProgram()
{
    static Program program;
    if(!program.mProgram)
    {
        program = Program("Circle.vsh", "Circle.fsh");
    }

    return program;
}

}
