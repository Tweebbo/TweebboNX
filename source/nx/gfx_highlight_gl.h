#pragma once
#include <SDL_opengles2.h>
#include "gfx_highlight.h"

#define HGL_PROCS(X) \
 X(GLuint,CreateShader,(GLenum)) X(void,ShaderSource,(GLuint,GLsizei,const GLchar *const*,const GLint*)) \
 X(void,CompileShader,(GLuint)) X(void,GetShaderiv,(GLuint,GLenum,GLint*)) X(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) \
 X(GLuint,CreateProgram,(void)) X(void,AttachShader,(GLuint,GLuint)) X(void,BindAttribLocation,(GLuint,GLuint,const GLchar*)) \
 X(void,LinkProgram,(GLuint)) X(void,GetProgramiv,(GLuint,GLenum,GLint*)) X(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) \
 X(void,DeleteShader,(GLuint)) X(void,DeleteProgram,(GLuint)) X(void,GenBuffers,(GLsizei,GLuint*)) X(void,DeleteBuffers,(GLsizei,const GLuint*)) \
 X(void,BindBuffer,(GLenum,GLuint)) X(void,BufferData,(GLenum,GLsizeiptr,const void*,GLenum)) X(void,BufferSubData,(GLenum,GLintptr,GLsizeiptr,const void*)) \
 X(GLint,GetUniformLocation,(GLuint,const GLchar*)) X(void,Uniform1i,(GLint,GLint)) X(void,Uniform4fv,(GLint,GLsizei,const GLfloat*)) \
 X(void,UseProgram,(GLuint)) X(void,GetIntegerv,(GLenum,GLint*)) X(GLboolean,IsEnabled,(GLenum)) X(void,Enable,(GLenum)) X(void,Disable,(GLenum)) \
 X(void,BlendFuncSeparate,(GLenum,GLenum,GLenum,GLenum)) X(void,BlendEquationSeparate,(GLenum,GLenum)) \
 X(void,EnableVertexAttribArray,(GLuint)) X(void,DisableVertexAttribArray,(GLuint)) X(void,VertexAttribPointer,(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*)) \
 X(void,GetVertexAttribiv,(GLuint,GLenum,GLint*)) X(void,GetVertexAttribPointerv,(GLuint,GLenum,void**)) X(void,DrawArrays,(GLenum,GLint,GLsizei)) X(void,ActiveTexture,(GLenum))
static struct {
#define HGL_MEMBER(ret,name,args) ret (GL_APIENTRY *name)args;
    HGL_PROCS(HGL_MEMBER)
#undef HGL_MEMBER
    GLuint program,buffer;GLint rows,base;bool ready;
} highlight_gl;

static GLuint highlight_gl_shader(GLenum type,const char *source){
    GLuint shader=highlight_gl.CreateShader(type);highlight_gl.ShaderSource(shader,1,&source,NULL);highlight_gl.CompileShader(shader);GLint ok=0;highlight_gl.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){char message[512];highlight_gl.GetShaderInfoLog(shader,sizeof message,NULL,message);dbg_log("Highlight shader: %s",message);highlight_gl.DeleteShader(shader);return 0;}return shader;
}
static bool highlight_gl_init(void){
#define HGL_LOAD(ret,name,args) do{highlight_gl.name=(ret(GL_APIENTRY*)args)SDL_GL_GetProcAddress("gl" #name);if(!highlight_gl.name){dbg_log("Highlight GL function missing: gl%s",#name);return false;}}while(0);
    HGL_PROCS(HGL_LOAD)
#undef HGL_LOAD
    const char *vertex="attribute vec2 position;attribute vec2 texcoord;varying vec2 uv;void main(){uv=texcoord;gl_Position=vec4(position,0.0,1.0);}";
    const char *fragment="precision highp float;varying vec2 uv;uniform sampler2D image;uniform vec4 rows[3];uniform vec4 base;void main(){vec4 pixel=texture2D(image,uv);vec4 value=vec4(pixel.rgb*base.rgb,1.0);gl_FragColor=vec4(clamp(vec3(dot(rows[0],value),dot(rows[1],value),dot(rows[2],value)),0.0,1.0),pixel.a*base.a);}";
    GLuint vs=highlight_gl_shader(GL_VERTEX_SHADER,vertex),fs=highlight_gl_shader(GL_FRAGMENT_SHADER,fragment);if(!vs||!fs){if(vs)highlight_gl.DeleteShader(vs);if(fs)highlight_gl.DeleteShader(fs);return false;}
    GLuint program=highlight_gl.CreateProgram();highlight_gl.AttachShader(program,vs);highlight_gl.AttachShader(program,fs);highlight_gl.BindAttribLocation(program,0,"position");highlight_gl.BindAttribLocation(program,1,"texcoord");highlight_gl.LinkProgram(program);highlight_gl.DeleteShader(vs);highlight_gl.DeleteShader(fs);
    GLint ok=0;highlight_gl.GetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){char message[512];highlight_gl.GetProgramInfoLog(program,sizeof message,NULL,message);dbg_log("Highlight shader link: %s",message);highlight_gl.DeleteProgram(program);return false;}
    highlight_gl.program=program;highlight_gl.rows=highlight_gl.GetUniformLocation(program,"rows[0]");highlight_gl.base=highlight_gl.GetUniformLocation(program,"base");
    GLint previous=0,buffer=0;highlight_gl.GetIntegerv(GL_CURRENT_PROGRAM,&previous);highlight_gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
    highlight_gl.UseProgram(program);highlight_gl.Uniform1i(highlight_gl.GetUniformLocation(program,"image"),0);highlight_gl.GenBuffers(1,&highlight_gl.buffer);highlight_gl.BindBuffer(GL_ARRAY_BUFFER,highlight_gl.buffer);highlight_gl.BufferData(GL_ARRAY_BUFFER,16*sizeof(float),NULL,GL_DYNAMIC_DRAW);highlight_gl.BindBuffer(GL_ARRAY_BUFFER,(GLuint)buffer);highlight_gl.UseProgram((GLuint)previous);highlight_gl.ready=true;return true;
}
static void highlight_gl_exit(void){if(highlight_gl.ready){highlight_gl.DeleteBuffers(1,&highlight_gl.buffer);highlight_gl.DeleteProgram(highlight_gl.program);highlight_gl.ready=false;}}
typedef struct {GLint enabled,size,type,normalized,stride,buffer;void *pointer;} HighlightAttribute;
static bool highlight_gl_draw(SDL_Renderer *renderer,SDL_Texture *texture,int sx,int sy,int sw,int sh,float x,float y,float width,float height,bool flip_x,bool flip_y,u32 color,const GfxHighlightMatrix *matrix,bool additive,int subtract){
    if(!highlight_gl.ready)return false;
    SDL_RenderFlush(renderer);GLint program,array_buffer,element_buffer,active,src_rgb,dst_rgb,src_alpha,dst_alpha,equation_rgb,equation_alpha;HighlightAttribute attribute[2];
    highlight_gl.GetIntegerv(GL_CURRENT_PROGRAM,&program);highlight_gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING,&array_buffer);highlight_gl.GetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element_buffer);highlight_gl.GetIntegerv(GL_ACTIVE_TEXTURE,&active);
    highlight_gl.GetIntegerv(GL_BLEND_SRC_RGB,&src_rgb);highlight_gl.GetIntegerv(GL_BLEND_DST_RGB,&dst_rgb);highlight_gl.GetIntegerv(GL_BLEND_SRC_ALPHA,&src_alpha);highlight_gl.GetIntegerv(GL_BLEND_DST_ALPHA,&dst_alpha);highlight_gl.GetIntegerv(GL_BLEND_EQUATION_RGB,&equation_rgb);highlight_gl.GetIntegerv(GL_BLEND_EQUATION_ALPHA,&equation_alpha);GLboolean blending=highlight_gl.IsEnabled(GL_BLEND);
    for(int index=0;index<2;index++){HighlightAttribute *a=&attribute[index];highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&a->enabled);highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_SIZE,&a->size);highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_TYPE,&a->type);highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,&a->normalized);highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_STRIDE,&a->stride);highlight_gl.GetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&a->buffer);highlight_gl.GetVertexAttribPointerv(index,GL_VERTEX_ATTRIB_ARRAY_POINTER,&a->pointer);}
    highlight_gl.ActiveTexture(GL_TEXTURE0);float tw=1,th=1;if(SDL_GL_BindTexture(texture,&tw,&th)){highlight_gl.ActiveTexture((GLenum)active);dbg_log("Highlight texture bind failed: %s",SDL_GetError());return false;}
    int texture_w,texture_h;SDL_QueryTexture(texture,NULL,NULL,&texture_w,&texture_h);
    float left=2*x/SCREEN_W-1,right=2*(x+width)/SCREEN_W-1,top=1-2*y/SCREEN_H,bottom=1-2*(y+height)/SCREEN_H;
    float u0=sx*tw/texture_w,u1=(sx+sw)*tw/texture_w,v0=sy*th/texture_h,v1=(sy+sh)*th/texture_h;if(flip_x){float swap=u0;u0=u1;u1=swap;}if(flip_y){float swap=v0;v0=v1;v1=swap;}
    const GLfloat vertices[16]={left,top,u0,v0,right,top,u1,v0,left,bottom,u0,v1,right,bottom,u1,v1};
    const GLfloat base[4]={(color&255)/255.f,(color>>8&255)/255.f,(color>>16&255)/255.f,(color>>24)/255.f};
    highlight_gl.UseProgram(highlight_gl.program);highlight_gl.Uniform4fv(highlight_gl.rows,3,&matrix->row[0][0]);highlight_gl.Uniform4fv(highlight_gl.base,1,base);highlight_gl.Enable(GL_BLEND);highlight_gl.BlendEquationSeparate(subtract?GL_FUNC_REVERSE_SUBTRACT:GL_FUNC_ADD,GL_FUNC_ADD);highlight_gl.BlendFuncSeparate(GL_SRC_ALPHA,additive||subtract?GL_ONE:GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    highlight_gl.BindBuffer(GL_ARRAY_BUFFER,highlight_gl.buffer);highlight_gl.BufferSubData(GL_ARRAY_BUFFER,0,sizeof vertices,vertices);highlight_gl.EnableVertexAttribArray(0);highlight_gl.EnableVertexAttribArray(1);highlight_gl.VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);highlight_gl.VertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));highlight_gl.DrawArrays(GL_TRIANGLE_STRIP,0,4);
    for(int index=0;index<2;index++){const HighlightAttribute *a=&attribute[index];highlight_gl.BindBuffer(GL_ARRAY_BUFFER,(GLuint)a->buffer);highlight_gl.VertexAttribPointer(index,a->size,(GLenum)a->type,(GLboolean)a->normalized,a->stride,a->pointer);if(a->enabled)highlight_gl.EnableVertexAttribArray(index);else highlight_gl.DisableVertexAttribArray(index);}
    highlight_gl.BindBuffer(GL_ARRAY_BUFFER,(GLuint)array_buffer);highlight_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)element_buffer);highlight_gl.UseProgram((GLuint)program);highlight_gl.BlendFuncSeparate((GLenum)src_rgb,(GLenum)dst_rgb,(GLenum)src_alpha,(GLenum)dst_alpha);highlight_gl.BlendEquationSeparate((GLenum)equation_rgb,(GLenum)equation_alpha);if(!blending)highlight_gl.Disable(GL_BLEND);SDL_GL_UnbindTexture(texture);highlight_gl.ActiveTexture((GLenum)active);return true;
}
#undef HGL_PROCS
