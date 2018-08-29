
#include <iostream>
#include <glad/glad.h>
#include "RenderDemo.h"
#include "macro.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL.h>
#include "char_unit.h"
#include <vector>
#include <stdio.h>
#include <list>
#include <tuple>
#include <type_traits>
#include <thread>
#include <chrono>

static int WORD_W = 10;
static int WORD_H = 18;

static int WORD_WX4 = 0; 

static int Window_width = 600;
static int Window_height = 500;

static float cursor_x = 0;
static float cursor_y = 0;

#define Word_Y 0.0f

#define HUI_CHE 28
#define KONG_GE 32
#define SUO_JIN 15 

#define GREEN glm::vec3(0.0f,1.0f,0.0f)
#define STR_COLOR glm::vec3(1.0f,0.5f,0.5f)
#define PRE_COLOR glm::vec3(0.2f,1.0f,0.7f) //预处理的颜色
#define KEY_COLOR glm::vec3(0.3f,0.2f,1.0f)
#define OTHER_COLOR glm::vec3(1.f,1.f,1.f)

template<typename T>
struct ArrLen;

template<typename T,size_t N>
struct ArrLen<T [N]>{
    static const int value =  N;
};

template<size_t N>
struct KW{
public:
    const wchar_t * const p;
    enum{ Len = N - 1};
    KW(const wchar_t *str) : p(str) {}
};

inline static int getTabW(int x);
inline static int getFullTabXLess(int x);

#define MK_KEYWORD(str)                      \
namespace ns_keywords{ const wchar_t _##str [] = L## #str; }

MK_KEYWORD(int)
MK_KEYWORD(if)
MK_KEYWORD(char)
MK_KEYWORD(long)
MK_KEYWORD(float)
MK_KEYWORD(double)
MK_KEYWORD(static)
MK_KEYWORD(const)
MK_KEYWORD(short)
MK_KEYWORD(struct)
MK_KEYWORD(sizeof)
MK_KEYWORD(return)
MK_KEYWORD(for)
MK_KEYWORD(while)
MK_KEYWORD(break)
MK_KEYWORD(continue)
MK_KEYWORD(else)
MK_KEYWORD(switch)
MK_KEYWORD(case)
MK_KEYWORD(unsigned)

#undef MK_KEYWORD

#define MK_KW(str) \
    KW< ArrLen< decltype( ns_keywords::_##str ) >::value >(ns_keywords::_##str) 

auto KW_TUP = std::make_tuple( 
    MK_KW(int)          ,
    MK_KW(if)           ,
    MK_KW(char)         ,
    MK_KW(long)         ,
    MK_KW(float)        ,
    MK_KW(double)       ,
    MK_KW(static)       ,
    MK_KW(const)        ,
    MK_KW(short)        ,
    MK_KW(struct)       ,
    MK_KW(sizeof)       ,
    MK_KW(return)       ,
    MK_KW(for)          ,
    MK_KW(while)        ,    
    MK_KW(break)        ,    
    MK_KW(continue)     ,    
    MK_KW(else)         ,
    MK_KW(switch)       ,
    MK_KW(case)         ,
    MK_KW(unsigned)         
   );

#undef MK_KW

template<size_t ...N>
auto get_kw_tuple(int index,std::index_sequence<N...> is) -> std::tuple<const wchar_t*,int>
{
    const wchar_t *ptr = nullptr;
    int len = 0;
    ((index == N && (ptr = std::get<N>(KW_TUP).p,len = std::remove_reference_t<decltype(std::get<N>(KW_TUP))>::Len ),false),...); 
    return std::make_tuple(ptr,len);
}

#define PI  3.1415926535898f 

static bool LeftButtonPressed = false;

static double LastCursorX = 0.0f;
static double LastCursorY = 0.0f;

static float WorldAngleX = 0.0f;
static float WorldAngleY = 0.0f;

static float WorldPosX = 0.0f;
static float WorldPosY = 0.0f;

static bool RightButtonPressed = false;

static float max_h = 0;

constexpr float PI_1_180()
{
    return PI/180.0f;
}

BuildStr(ShaderSrc,vertex, #version 450 core\n
    uniform mat4 ortho; \n
    uniform mat4 world; \n
    uniform mat4 model; \n
    layout(location = 0) in vec3 vposition; \n
    layout(location = 1) in vec2 texCoord; \n
    out vec2 TexCoord;\n
    void main() \n
    { \n
	    gl_Position = ortho * world * model * vec4( vposition, 1); \n
        TexCoord = texCoord;\n
    }\n
)

BuildStr(ShaderSrc,fragment, #version 450 core \n
    in vec2 TexCoord;\n
    out vec4 color;  \n
    uniform sampler2D ourTexture;\n
    uniform vec3 Color;\n
    void main()\n
    {\n
	    color = texture(ourTexture, TexCoord); \n
        if(color.a != 0.0)
        {
            color = vec4(Color,color.a);
        }
    }\n
)

BuildStr(ShaderSrc,line_vs, #version 450 core\n
    uniform mat4 ortho; \n
    uniform mat4 world; \n
    uniform mat4 model; \n
    layout(location = 0) in vec3 vposition; \n
    void main() \n
    { \n
	    gl_Position = ortho * world * model * vec4( vposition, 1); \n
    }\n
)

BuildStr(ShaderSrc,line_fg, #version 450 core \n
    out vec4 color;  \n
    uniform vec3 Color;\n
    void main()\n
    {\n
	    color = vec4(Color,1.0f); \n
    }\n
)

struct Plane {
    glm::vec3 pos;
    glm::vec2 tc;
};

struct Word{
public:
    Word(glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_){
        this->pos = pos_;
        this->color = color_;
        this->angle = angle_;
        this->c = c_;
        this->w = w_;
        this->h = h_;
    }
    Word() : pos(),
            color(),
            angle(),
            c(0),
            w(0),
            h(0)
    {
    }
    bool isSpace()
    {
        return this->c == HUI_CHE || this->c == KONG_GE || this->c == SUO_JIN;
    }
    bool inBound(int x,int y,int w_,int h_)
    {
        int rbx = pos.x + this->w,rby = pos.y + this->h; // rb is right and bottom
        return pos.x >= x && pos.y >= y && pos.x <= (x + w_) && pos.y <= (y + h_)  ||  
                rbx >= x && rby >= y && rbx <= (x + w_) && rby <= (y + h_);
    }
    Word(const Word& w) {
        this->pos =     w.pos;
        this->color =   w.color;
        this->angle =   w.angle;
        this->c =       w.c;
        this->w =       w.w;
        this->h =       w.h;
    }
    Word(Word&& w) {
        this->pos =     w.pos;
        this->color =   w.color;
        this->angle =   w.angle;
        this->c =       w.c;
        this->w =       w.w;
        this->h =       w.h;
    }
    Word& operator=(Word& w){
        this->pos =     w.pos;
        this->color =   w.color;
        this->angle =   w.angle;
        this->c =       w.c;
        this->w =       w.w;
        this->h =       w.h;
        return *this;
    }
    glm::vec3 pos;
    glm::vec3 angle;
    glm::vec3 color;
    wchar_t c;
    int w,h;
};

std::list<Word*> animate_list;
std::list<Word>::iterator *pit = nullptr;

class Demo1 : public RenderDemo{
public:
    virtual int init() override{

        // float data[] = {
        //     0.0f,-0.8f,0.0f,    0.5f,1.0f,
        //     0.8f,0.8f,0.0f,     1.0f,0.0f,
        //     -0.8f,0.8f,0.0f,    0.0f,0.0f,
        // };
        planes.reserve(30);

        FILE *file = fopen("../../res/test.txt","rb");
        if(file)
        {
            int v = 0;
            char_unit::CharUnit cu;
            while((v = read_file(&cu,file)) != -1)
            {
                if(cu.h > WORD_H)
                {
                    WORD_H = cu.h;
                    WORD_W = cu.w;
                }
                cus.push_back(cu);
            }
            fclose(file);
        }
        max_h = WORD_H;
        WORD_WX4 = WORD_W * 4;

        glfwSetCharCallback(m_window,Demo1::CharCallBack);
        glfwSetCharModsCallback(m_window,Demo1::CharModsCallBack);
        glfwSetKeyCallback(m_window,Demo1::KeyCallBack);
        glfwSetWindowSizeCallback(m_window,Demo1::WindowResize);
        glfwSetMouseButtonCallback(m_window,Demo1::MouseButtonCallBack);
        glfwSetCursorPosCallback(m_window,Demo1::CursorCallBack);
        glfwSetWindowFocusCallback(m_window,Demo1::WindowFocusCallBack);
    
        glEnable (GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glGenVertexArrays(1,&vertex_array);
        
        glBindVertexArray(vertex_array);

        glGenBuffers(1,&vertex_buffer);

        //EBO Buffer 
        glGenBuffers(1,&word_ebo);
        //EBO Buffer  --------------------------------------------
        glBindVertexArray(0);

        glGenVertexArrays(1,&line_array);
        glBindVertexArray(line_array);
        glGenBuffers(1,&vernier_buffer);

        float line_data[] = { 0.0f,0.0f,0.0f,0.0f,1.0f,0.0f};

        glBindBuffer(GL_ARRAY_BUFFER,vernier_buffer);
        
        glBufferData(GL_ARRAY_BUFFER,sizeof(line_data),line_data,GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,0);

       // glBindBuffer(GL_ARRAY_BUFFER,vernier_buffer);
       // glBufferData(GL_ARRAY_BUFFER,sizeof(data),data,GL_STATIC_DRAW);

        //vposition = 0;
        //glVertexAttribPointer(vposition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
	    //glEnableVertexAttribArray(vposition);

        //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(sizeof(GLfloat) * 3) );
	    //glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        glGenTextures(1,&texture0);
        glBindTexture(GL_TEXTURE_2D,texture0); 

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        
        unsigned char *image_data = SOIL_load_image("../../res/test.png",&char_map_w,&char_map_h,0,SOIL_LOAD_RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, char_map_w, char_map_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        SOIL_free_image_data(image_data);
        glBindTexture(GL_TEXTURE_2D,0);

        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	    glShaderSource(vertex_shader, 1 ,&(_ShaderSrc::_vertex), NULL);
	    glCompileShader(vertex_shader);

        GLint compile_status = GL_TRUE;
        char buf[1024] = {0};
        int size = 0;

        glGetShaderiv(vertex_shader,GL_COMPILE_STATUS,&compile_status);
        if(!compile_status)
        {
            glGetShaderInfoLog(vertex_shader,sizeof(buf),&size,buf);
            printf("%s\n",buf);
        }

	    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	    glShaderSource(fragment_shader, 1, &(_ShaderSrc::_fragment), NULL);
	    glCompileShader(fragment_shader);

        glGetShaderiv(fragment_shader,GL_COMPILE_STATUS,&compile_status);
        if(!compile_status)
        {
            memset(buf,0,sizeof(buf));
            glGetShaderInfoLog(fragment_shader,sizeof(buf),&size,buf);
            printf("%s\n",buf);
        }
        

	    program = glCreateProgram();
	    glAttachShader(program, vertex_shader);
	    glAttachShader(program, fragment_shader);
	    glLinkProgram(program);

        

	    glUseProgram(program);

	    ortho =     glGetUniformLocation(program, "ortho");
	    world =     glGetUniformLocation(program, "world");
	    model =     glGetUniformLocation(program, "model");
        tex0 =      glGetUniformLocation(program,"ourTexture");
        ucolor =    glGetUniformLocation(program,"Color");

        //printf("init %d %d %d %d\n",ucolor,ortho,world,model);

        line_vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(line_vs,1,&(_ShaderSrc::_line_vs),NULL);
        glCompileShader(line_vs);

        glGetShaderiv(line_vs,GL_COMPILE_STATUS,&compile_status);
        if(!compile_status)
        {
            memset(buf,0,sizeof(buf));
            glGetShaderInfoLog(line_vs,sizeof(buf),&size,buf);
            printf("%d , %s\n",size,buf);
        }


        line_fg = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(line_fg,1,&(_ShaderSrc::_line_fg),NULL);
        glCompileShader(line_fg);

        glGetShaderiv(line_fg,GL_COMPILE_STATUS,&compile_status);
        if(!compile_status)
        {
            memset(buf,0,sizeof(buf));
            glGetShaderInfoLog(line_fg,sizeof(buf),&size,buf);
            printf("%d %s\n",size,buf);
        }

        line_program = glCreateProgram();
        glAttachShader(line_program,line_vs);
        glAttachShader(line_program,line_fg);
        glLinkProgram(line_program);

        glUseProgram(line_program);

        line_ortho =     glGetUniformLocation(line_program, "ortho");
	    line_world =     glGetUniformLocation(line_program, "world");
	    line_model =     glGetUniformLocation(line_program, "model");
        line_color =    glGetUniformLocation(line_program,"Color");

        printf("init %d %d %d %d\n",line_color,line_ortho,line_world,line_model);

        glm::vec3 line_col(1.0f,1.0f,1.0f);
        glUniform3fv(line_color,1,glm::value_ptr(line_col));

        glClearColor(0.0f,0.0f,0.0f,1.0f);

        GLenum err = glGetError();
        printf("err = %d \n",err);

        return 0;
    }

    void push_back_word(glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word w(pos_,angle_,color_,c_,w_,h_);
        words.push_back(w);
        if(!w.isSpace())
        {
            push_back_plane(w);
            animate_list.push_back(&(words.back()));
        }
        reSetVernierClock();
        needUpdateColour = true;
        reDraw();
    }
    void push_back_word_na(glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word w(pos_,angle_,color_,c_,w_,h_);
        words.push_back(w);
        if(!w.isSpace())
        {
            push_back_plane(w);
        }
    }

    Word pop_end_word()
    {
        if(words.empty())
            return Word();
        Word *ptr = &(words.back());
        for(auto it = animate_list.begin();it != animate_list.end();)
        {
            if(*it == ptr)
            {
                it = animate_list.erase(it);
                continue;
            }
            ++it;
        }
        Word wor = *ptr;
        words.pop_back();
        if(!wor.isSpace())
        {
            planes.pop_back();
            planes.pop_back();
            planes.pop_back();
            planes.pop_back();
            //planes.pop_back();
            //planes.pop_back();
        }
        reSetVernierClock();
        needUpdateColour = true;
        reDraw();
        return wor; 
    }
    void insert_back_it_no(std::list<Word>::iterator &it,glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word w(pos_,angle_,color_,c_,w_,h_);
        it = words.insert(it,w);
        ++it;
    }
    void adjustPos(std::list<Word>::iterator last,std::list<Word>::iterator next)
    {
        bool f = false;
        int bx,by;
        if(last->c == HUI_CHE)
            f = true;
        if(f)
        {
            bx = 0;
            by = last->pos.y + last->h; 
        }else{
            bx = last->pos.x + last->w;
            by = last->pos.y; 
        }
        while(true)
        {
            if(next == words.end())
                break;
            while(true)
            {
                if(next == words.end())
                    goto adjustPos_END;

                next->pos.x = bx;
                next->pos.y = by;
                bx += next->w;
                if(next->c == HUI_CHE)
                {
                    bx = 0;
                    by = by + next->h;
                    ++next;
                    break;
                }
                ++next;
            }    
        }
        adjustPos_END: ;
    }
    void insert_back_tab_it(std::list<Word>::iterator &it,Word &w)
    {
        int right0 = w.pos.x + w.w;
        int right1 = it->pos.x + it->w;
        if(right0 >= right1)
        {
            it->pos.x = right0;
            it->w = getTabW(right0);
            it = words.insert(it,w);

            std::list<Word>::iterator n_it = ++it;
            int zl = (right0 - right1) + it->w;
            while(true)
            {
                n_it++;
                if(n_it == words.end())
                {
                    break;
                }
                n_it->pos.x += zl;
                if(n_it->c == HUI_CHE)
                {
                    break;
                }
            }
        }else{
            it->pos.x = right0;
            it->w -= w.w;
            it = words.insert(it,w);
            ++it;
            return;
        }            
    }
    void insert_back_it(std::list<Word>::iterator &it,glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word w(pos_,angle_,color_,c_,w_,h_);
        if(it->c == SUO_JIN)
        {
            insert_back_tab_it(it,w);
            needUpdateColour = true;
            reDraw();
            if(c_ != KONG_GE || c_ != SUO_JIN)
            {
                needUpdatePlanes = true;
            }
            return;
        }
        it = words.insert(it,w);
        std::list<Word>::iterator n_it = it;
        int bx = n_it->pos.x + n_it->w;
        while(true)
        {
            n_it++;
            if(n_it == words.end())
            {
                break;
            }
            n_it->pos.x = bx;
            if(n_it->c == HUI_CHE)
            {
                break;
            }
            if(n_it->c == SUO_JIN)
                n_it->w = getTabW(bx);
            bx += n_it->w;
        }
        it++;
        reSetVernierClock();
        if(c_ != KONG_GE || c_ != SUO_JIN)
        {
            needUpdatePlanes = true;
        }
        needUpdateColour = true;
        reDraw();
    }
    void insert_return_back_it(std::list<Word>::iterator &it,glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word space(pos_,angle_,color_,c_,w_,h_);
        it = words.insert(it,space);
        std::list<Word>::iterator n_it = it;
        while(true)
        {
            n_it++;
            if(n_it == words.end())
            {
                break;
            }
            n_it->pos.x -= pos_.x;
            n_it->pos.y += h_;
            if(n_it->c == HUI_CHE)
            {
                break;
            }
        }
        while(n_it != words.end())
        {
            n_it++;
            n_it->pos.y += h_;
        }
        it++;
        reSetVernierClock();
        needUpdateColour = true;
        reDraw();
    }
    void remove_forword_it(std::list<Word>::iterator &it)
    {
        bool f = false;
        bool itIsTab = false;
        std::list<Word>::iterator tab_it;
        int x = it->pos.x;
        int y = it->pos.y;
        if(it->c == SUO_JIN)
        {
            itIsTab = true;
            tab_it = it;
        }
        --it;
        int offsetx = it->pos.x - x;
        int offsety = it->pos.y - y;
        if(it->c == HUI_CHE)
            f = true;

        if(itIsTab)
        {
            if(!f && it->pos.x >= getFullTabXLess(tab_it->pos.x) )
            {
                bool temp = it->isSpace();
                tab_it->pos.x = it->pos.x;
                tab_it->w += it->w;
                it = words.erase(it);
                if(!temp)
                     needUpdatePlanes = true;
                needUpdateColour = true;
                reDraw();
                reSetVernierClock();
                return;
            }else{
                tab_it->pos = it->pos;
                tab_it->w = getTabW(tab_it->pos.x);
                it = words.erase(it);
                ++tab_it;
                printf("%d   %d\n",it->c,tab_it->c);
                adjustPos(it,tab_it);
                needUpdatePlanes = true;
                needUpdateColour = true;
                reDraw();
                reSetVernierClock();
                return;
            }
        }
        
        int bx = it->pos.x;
        int by = it->pos.y;
        it = words.erase(it);
        std::list<Word>::iterator n_it = it;
        while(true)
        {
            if(n_it == words.end())
            {
                break;
            }
            n_it->pos.x = bx;
            n_it->pos.y = by;
            if(n_it->c == HUI_CHE)
            {
                if(f)
                {
                    ++n_it;
                    if(n_it == words.end())
                        break;
                    offsetx = x - n_it->pos.x;
                    offsety = y - n_it->pos.y;
                    while(true)
                    {
                        if(n_it == words.end())
                        {
                            break;
                        }
                        n_it->pos.x += offsetx;
                        n_it->pos.y += offsety;
                        ++n_it;    
                    }
                }
                break;
            }
            if(n_it->c == SUO_JIN)
                n_it->w = getTabW(bx);
            bx += n_it->w;
            n_it++;
        }
        reSetVernierClock();
        //printf("%d",it->c);
        
        needUpdatePlanes = true;
        needUpdateColour = true;
        reDraw();
    }
    void get_less_xy_it(std::list<Word>::iterator &it,int x,int y)
    {
        while(true)
        {
            --it;
            if(it->pos.x <= x && it->pos.y < y)
                return;
        }
    }
    void get_more_xy_it(std::list<Word>::iterator &it,int x,int y)
    {
        for(;it != words.end();)
        {
            ++it;
            if(it->pos.y > y)
             {
                 if(it == words.end())
                    return;
                else if(it->c == HUI_CHE ){
                    return;
                }else if(it->pos.x >= x)
                {
                    return;
                }
             }   
        }
    }
    bool eq(int x1,int x2,int w)
    {
        return x1 >= x2 && (x1 - x2) <= w; 
    }
    void get_xy_it(std::list<Word>::iterator &it,int x,int y)
    {
        if(it == words.end())
            --it;
        bool dy_y = false; //大于y
        bool dy_x = false;
        if(y >= it->pos.y)
            dy_y = true;
        if(x >= it->pos.x)
            dy_x = true;
        
        if(eq(y,(int)it->pos.y,max_h))
        {
            if(dy_x)
            {
                while(it != words.end())
                {
                    if(eq(x,(int)it->pos.x,it->w))
                    {   
                        ++it; 
                        return;
                    }
                    ++it;
                    if(it == words.end() || it->c == HUI_CHE)
                    {
                        it = words.end();
                        break;
                    }
                }
            }else{
                while(it != words.end())
                {
                    if(eq(x,(int)it->pos.x,it->w))
                    {   
                        ++it; 
                        return;
                    }
                    --it;
                    if(it == words.end() || it->c == HUI_CHE)
                    {
                        it = words.end();
                        break;
                    }
                }
            }
        }else{
            if(dy_y)
            {
                while(it != words.end())
                {
                    if(eq(x,(int)it->pos.x,it->w) && eq(y,(int)it->pos.y,max_h))
                    {   
                        ++it; 
                        return;
                    }
                    ++it;
                }
            }else{
                while(it != words.end())
                {
                    if(eq(x,(int)it->pos.x,it->w) && eq(y,(int)it->pos.y,max_h))
                    {   
                        ++it; 
                        return;
                    }
                    --it;
                }
            }
        }
    }
    std::list<Word>::iterator get_last_it()
    {
        auto it = words.end();
        --it;
        return it;
    }
    const char_unit::CharUnit *getcu(wchar_t c)
    {
        for(auto &cu: cus)
        {
            if(cu.c == c)
            {
                return &cu;
            }
        }
        return nullptr;
    }
    virtual void run() override {
        while (!glfwWindowShouldClose(m_window))
	    {
            if(hasFocus)
            {
                auto now_time = std::chrono::system_clock::now();
                auto interval = now_time - vernier_clock;
                if( std::chrono::duration_cast<std::chrono::seconds>(interval).count() >= 1)
                {
                    vernier_clock = std::chrono::system_clock::now();
                    needDrawVernier = !needDrawVernier;
                    needReDraw = true;
                }
            }

            if(needReDraw)
            {
                needReDraw = false;
                draw();
                glfwSwapBuffers(m_window);
            }  
            
		    glfwPollEvents();
            //std::this_thread::sleep_for(std::chrono::microseconds(5));
	    }
    }
    void reDraw()
    {
        needReDraw = true;
    }
    void reSetVernierClock()
    {
        needDrawVernier = true;
        vernier_clock = std::chrono::system_clock::now();
    }
    virtual ~Demo1() 
    {
        destroy();
    }
protected:

    void draw_char_unit(Word &w,int index)
    {
        glm::mat4 model_mat;
        model_mat = glm::scale(model_mat,glm::vec3(1.0f,1.0f,1.0f));
        model_mat = glm::translate(model_mat,glm::vec3(w.pos.x + ((float)w.w)/2.0f ,
                                                        w.pos.y + ((float)w.h)/2.0f,
                                                        w.pos.z
                                                        ));
        model_mat = glm::rotate(model_mat,PI_1_180() * w.angle.x,glm::vec3(1.0f,0.0f,0.0f));
        model_mat = glm::rotate(model_mat,PI_1_180() * w.angle.y,glm::vec3(0.0f,1.0f,0.0f));
        model_mat = glm::rotate(model_mat,PI_1_180() * w.angle.z,glm::vec3(0.0f,0.0f,1.0f));

		glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(model_mat));
        glUniform3fv(ucolor,1,glm::value_ptr(w.color));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,word_ebo);
        int zl = index << 2;
        GLuint index_buf[] = { 0 + zl ,2 + zl,3 + zl,3 + zl,1 + zl,0 + zl  };

        glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(index_buf),index_buf,GL_STATIC_DRAW);

        //glDrawArrays(GL_TRIANGLES, index, 6);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    }
    void draw_char()
    {
        if(needUpdateColour)
        {
            colour_word();
            needUpdateColour = false;
        }
        if(needUpdatePlanes)
        {
            planes.clear();
            for(auto &w : words)
            {
                if(w.isSpace())
                    continue;
                push_back_plane(w);
            }
            needUpdatePlanes = false;
        }
        glBindBuffer(GL_ARRAY_BUFFER,vertex_buffer);

        glBufferData(GL_ARRAY_BUFFER,planes.size() * sizeof(Plane),planes.data(),GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Plane), (void *)0);
	    glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Plane), (void *)12);
	    glEnableVertexAttribArray(1);

        int index = 0;
        for(auto &w : words)
        {
            if(w.isSpace())
                continue;
            if(w.inBound(0 - WorldPosX, 0 - WorldPosY, Window_width,Window_height))
                draw_char_unit(w,index);
            index += 1;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER,0);
    }
    void push_back_plane(Word& w)
    {
        auto cu = getcu(w.c);
        if(!cu)
            return;
        float ax,ay,bx,by;

        ax = (float)cu->x / (float)char_map_w * 1.0f;
        ay = (float)cu->y / (float)char_map_h * 1.0f;
        bx = (float)(cu->x + cu->w) / (float)char_map_w * 1.0f;
        by = (float)(cu->y + cu->h) / (float)char_map_h * 1.0f;

        //printf("%f %f %f %f\n",ax,ay,bx,by);
        float half_w = ((float)w.w) / 2.0f;
        float half_h = ((float)w.h) / 2.0f;
        Plane _1,_2,_3,_4;
        _1.pos = glm::vec3(-half_w,-half_h,0.0f);
        _1.tc = glm::vec2(ax,ay);

        _2.pos = glm::vec3(half_w ,-half_h,0.0f);
        _2.tc = glm::vec2(bx,ay);

        _3.pos = glm::vec3(-half_w , half_h ,0.0f);
        _3.tc = glm::vec2(ax,by);

        _4.pos = glm::vec3(half_w , half_h,0.0f);
        _4.tc = glm::vec2(bx,by);

        planes.push_back(_1);
        planes.push_back(_2);
        planes.push_back(_3);
        planes.push_back(_4);
        //planes.push_back(_2);
        //planes.push_back(_1);
    }

    void drawVernier(glm::mat4 &ortho_mat,glm::mat4 &world_mat)
    {
        glUseProgram(line_program);
        glBindVertexArray(line_array);
        glBindBuffer(GL_ARRAY_BUFFER,vernier_buffer);

        glUniformMatrix4fv(line_ortho , 1, GL_FALSE, glm::value_ptr(ortho_mat));
        glUniformMatrix4fv(line_world , 1, GL_FALSE, glm::value_ptr(world_mat));

        glm::mat4 model_mat;
        model_mat = glm::translate(model_mat,glm::vec3(cursor_x + 1.0f ,cursor_y,Word_Y - 5.0f));
        model_mat = glm::scale(model_mat,glm::vec3(1.0f,WORD_H,1.0f));
        glUniformMatrix4fv(line_model , 1, GL_FALSE, glm::value_ptr(model_mat));
        
        glDrawArrays(GL_LINES,0,2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

#define END_BREAK(it)           \
        if(it == words.end())   \
                goto NOSTEP;     

    void colour_word()
    {
        std::list<Word>::iterator it = words.begin();
        for(;it != words.end();)
        {
            it->color = OTHER_COLOR;
            switch(it->c)
            {
                case L'/':
                {
                   Word *now = &(*it);
                   ++it;
                   END_BREAK(it)
                   it->color = OTHER_COLOR;
                   if(it->c == L'/')
                   {
                       now->color = GREEN;
                       it->color = GREEN;
                       do{
                           ++it;
                           END_BREAK(it)
                           it->color = GREEN;
                       }
                       while(it->c != HUI_CHE);
                   } else if(it->c == L'*')
                   {
                       now->color = GREEN;
                       it->color = GREEN;
                       do{
                           ++it;
                           END_BREAK(it)
                           it->color = GREEN;
                           if(it->c == L'*')
                           {
                               ++it;
                               END_BREAK(it)
                               it->color = GREEN;
                               if(it->c == L'/')
                               {
                                   break;
                               }
                           }
                       }
                       while(true);
                   }
                   break; 
                } 
                case L'"':
                {
                    it->color = STR_COLOR;
                    do{
                        ++it;
                        END_BREAK(it)
                        it->color = STR_COLOR;
                    }
                    while(it->c != L'"');
                }
                break;  
                case L'#':
                {
                    it->color = PRE_COLOR;
                    do{
                        ++it;
                        END_BREAK(it)
                        it->color = PRE_COLOR;
                    }
                    while(it->c != HUI_CHE);
                }
                break;
                default:
                {
                    if(it != words.begin())
                    {
                        --it;
                        if(! (it->isSpace() || it->c == L'(' || it->c == L';' || it->c == L'{' || it->c == L','  || it->c == L'}') )
                        {
                            ++it;
                            break;
                        }
                        ++it;
                    }
                    
                    std::list<Word>::iterator src_it = it;
                    bool f = true;
                    for(int i = 0;i < std::tuple_size<decltype(KW_TUP)>::value;++i)
                    {
                        f = true;
                        auto [ptr,len] = get_kw_tuple(i,std::make_index_sequence<std::tuple_size<decltype(KW_TUP)>::value>()); 
                        for(int j = 0;j < len;++j)
                        {
                            it->color = OTHER_COLOR;
                            if(ptr[j] != it->c)
                            {
                                f = false;
                                it = src_it;
                                break;
                            }
                            ++it;
                            END_BREAK(it)
                        }
                        if(f)
                        {
                            if(! (it->isSpace() || it->c == L'}' || it->c == L')' || it->c == L'(' || it->c == L';' || it->c == L'{' ) )
                            {
                                it = src_it;
                                break; 
                            }
                            it = src_it;
                            for(int j = 0;j < len;++j)
                            {
                                it->color = KEY_COLOR;
                                ++it;
                            }
                            goto NOSTEP;
                            break; 
                        } 
                    }
                }
                break; 
            }
            ++it;
NOSTEP:         ;
        }
    }
#undef END_BREAK

    virtual void draw() override{
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vertex_array);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture0);
        glUniform1i(tex0,0);

        
        glm::mat4 ortho_mat = glm::ortho(0.0f, (float)Window_width, (float)Window_height, 0.0f,-1000.0f,1000.0f);
	    glUniformMatrix4fv(ortho , 1, GL_FALSE, glm::value_ptr(ortho_mat));
        
        glm::mat4 world_mat;
        world_mat = glm::scale(world_mat,glm::vec3(1.0f));
        world_mat = glm::translate(world_mat,glm::vec3(WorldPosX,WorldPosY,0.0f));
        world_mat = glm::rotate(world_mat,WorldAngleX,glm::vec3(1.0f,0.0f,0.0f));
        world_mat = glm::rotate(world_mat,WorldAngleY,glm::vec3(0.0f,1.0f,0.0f));
        
		glUniformMatrix4fv(world, 1, GL_FALSE, glm::value_ptr(world_mat));
        draw_char();
        if(needDrawVernier)
            drawVernier(ortho_mat,world_mat);
        // m_a += sinf(m_s) ;
        // if(m_a >= 360.0f)
        // {
        //     m_s = 0.0f;
        //     m_a = 0.0f;
        // }else{
        //     m_s += 0.01f;
        // }
        
        if(!animate_list.empty())
            needReDraw = true;
        for(auto it = animate_list.begin();it != animate_list.end();)
        {
            if((*it)->angle.z >= 360.0f)
            {
                (*it)->angle.z = 0.0f;
                (*it)->angle.y = 0.0f;
                it = animate_list.erase(it);
                continue;
            }else{
                (*it)->angle.z += 16.0f;
                (*it)->angle.y += 16.0f;
            }
            ++it;
        }
    }
    
    virtual void destroy() override{
        glDeleteTextures(1,&texture0);
        glDeleteBuffers(1,&vernier_buffer);
        glDeleteBuffers(1,&word_ebo);
        glDeleteBuffers(1,&vertex_buffer);
        glDeleteVertexArrays(1,&vertex_array);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(program);
        glDeleteShader(line_vs);
        glDeleteShader(line_fg);
        glDeleteProgram(line_program);
    }
    static void CharCallBack(GLFWwindow* w,unsigned int v);
    static void CharModsCallBack(GLFWwindow*,unsigned int,int);
    static void KeyCallBack(GLFWwindow*,int,int,int,int);
    static void WindowResize(GLFWwindow*,int,int);
    static void MouseButtonCallBack(GLFWwindow*,int,int,int);
    static void CursorCallBack(GLFWwindow*,double,double);
    static void WindowFocusCallBack(GLFWwindow*,int);
private:
    GLuint vertex_array,vertex_shader,fragment_shader,texture0,line_vs,line_fg,line_program,line_array;
    GLuint vertex_buffer,vernier_buffer,word_ebo;
    GLuint program;
    GLuint ucolor,ortho,world,model,tex0,line_ortho,line_world,line_model,line_color;
    //float m_a = 0.0f;
    //float m_s = 0.0f;
    int char_map_w,char_map_h; 
    std::vector<Plane> planes;
    std::list<Word> words;
    std::vector<char_unit::CharUnit> cus;
    bool needUpdatePlanes = false;
    bool needUpdateColour = false;
    bool needDrawVernier = true;
    bool needReDraw = true;
    bool hasFocus = true;
    std::chrono::system_clock::time_point vernier_clock;
};


static Demo1 *demo = nullptr;

void Demo1::CharCallBack(GLFWwindow* w,unsigned int v)
{
    auto cu = demo->getcu(v);
    if(!cu)
        return;
    if(pit == nullptr || *pit == demo->words.end())
    {
        demo->push_back_word(glm::vec3(cursor_x,cursor_y,Word_Y),
                glm::vec3(0.0f,0.0f,0.0f),
                glm::vec3(1.0f,1.0f,1.0f),
                v,cu->w,cu->h);
    }else{
        demo->insert_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                glm::vec3(0.0f,0.0f,0.0f),
                glm::vec3(1.0f,1.0f,1.0f),
                v,cu->w,cu->h);
    }
    cursor_x += cu->w;
    if(cu->h > max_h)
    {
        max_h = cu->h;
    }
    //printf("%d\n",v);
}

void Demo1::CharModsCallBack(GLFWwindow*,unsigned int v1,int v2)
{
    //printf("Char Mods %d %d\n",v1,v2);
    switch(v1)
    {
        case KONG_GE:
            if(pit == nullptr || *pit == demo->words.end())
            {
                demo->push_back_word(glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            KONG_GE,WORD_W,max_h);
            }else{
                demo->insert_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            KONG_GE,WORD_W,max_h);
            }
            cursor_x += WORD_W;
        break;
    }
}
static bool CtrlDown = false;

inline static int getTabW(int x)
{
    int m = x % WORD_WX4;
    if(m == 0)
        return WORD_WX4;
    else
        return WORD_WX4 - m; 
}

inline static int getFullTabXLess(int x)
{
    return x - (x % WORD_WX4);
}

void Demo1::KeyCallBack(GLFWwindow*,int v1,int v2,int v3,int v4)
{
    //printf("Key %d %d %d %d\n",v1,v2,v3,v4);
    if(v1 == 341)
    {
        if(v3 == 1)
        {
            CtrlDown = true;
        }else if(v3 == 0)
        {
            CtrlDown = false;
        }
    }else if(v3 == 1 || v3 == 2)
    {
        switch(v2)
        {
            case HUI_CHE:
                if(pit == nullptr || *pit == demo->words.end())
                {
                    demo->push_back_word(glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            HUI_CHE,6,max_h);
                
                }else{
                    demo->insert_return_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            HUI_CHE,6,max_h);
                }
                cursor_y += max_h;
                cursor_x = 0;
            break;
            case 14:
                if(cursor_x <= 0 && cursor_y <= 0)
                    return;
                if(pit == nullptr || *pit == demo->words.end()){
                    Word wor = demo->pop_end_word();
                    cursor_x = wor.pos.x;
                    cursor_y = wor.pos.y;
                }else{
                    demo->remove_forword_it(*pit);
                    cursor_x = (*pit)->pos.x;
                    cursor_y = (*pit)->pos.y;
                }
            break;
            case SUO_JIN:
                {
                    int tab_w = getTabW(cursor_x);
                    if(pit == nullptr || *pit == demo->words.end())
                    {
                         demo->push_back_word(glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                SUO_JIN,tab_w,max_h);
                    }else{
                        demo->insert_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                SUO_JIN,tab_w,max_h);
                    }
                    cursor_x += tab_w;    
                    break;
                }
            case 331: // left 
                if(cursor_x <= 0 && cursor_y <= 0)
                    return;
                {
                   if(!pit)
                   {
                       pit = new std::list<Word>::iterator(demo->get_last_it());
                   }else
                        --(*pit);
                    
                    cursor_x = (*pit)->pos.x;
                    cursor_y = (*pit)->pos.y;
                    demo->reDraw();
                    demo->reSetVernierClock();
                }
            break;
            case 333:  // right
            
                if(demo->words.empty() || *pit == demo->words.end() || ( cursor_x > demo->words.back().pos.x && cursor_y == demo->words.back().pos.y ) )
                {
                    return;
                }
                demo->reDraw();
                demo->reSetVernierClock();
                ++(*pit);
                if(*pit == demo->words.end())
                {
                    --(*pit);
                    if((*pit)->c == HUI_CHE)
                    {
                        cursor_y += (*pit)->h;
                        cursor_x = 0;
                         ++(*pit);
                        return ;
                    }
                    cursor_x += (*pit)->w;
                    ++(*pit);
                    return ;
                }
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
            break;
            case 328:  // up
                if(cursor_y == 0)
                    return;
                if(!pit)
                    pit = new std::list<Word>::iterator(demo->get_last_it());
                demo->get_less_xy_it(*pit,cursor_x,cursor_y); 
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->reDraw();
                demo->reSetVernierClock();
            break;
            case 336:  // down
                if( demo->words.empty())
                    return;
                else if(cursor_y == demo->words.back().pos.y){
                    if(demo->words.back().c == HUI_CHE)
                    {
                        cursor_y += demo->words.back().h;
                        *pit = demo->words.end();
                        cursor_x = 0;
                        demo->reDraw();
                        demo->reSetVernierClock();
                    }
                    return;
                }else if(cursor_y > demo->words.back().pos.y)
                    return;
                demo->get_more_xy_it(*pit,cursor_x,cursor_y); 
                if(*pit == demo->words.end())
                {
                    --(*pit);
                    cursor_x = (*pit)->pos.x + (*pit)->w;
                    cursor_y = (*pit)->pos.y;
                    demo->reDraw();
                    demo->reSetVernierClock();
                    ++(*pit);
                    return ;
                }
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->reDraw();
                demo->reSetVernierClock();
            break;
        }
    }
    if(CtrlDown && v3 == 1) // Ctrl v
    {
        if(v2 == 47)
        {
            const char * p = glfwGetClipboardString(demo->m_window);
            
            if(pit == nullptr || *pit == demo->words.end())
            {
                bool allIsSpace = true;
                while(*p)
                {
                    switch(*p)
                    {
                        case ' ':
                            demo->push_back_word_na(glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                KONG_GE,WORD_W,max_h);
                            cursor_x += WORD_W;
                        break;
                        case '\t':
                            {   int tab_w = getTabW(cursor_x);
                                demo->push_back_word_na(glm::vec3(cursor_x,cursor_y,Word_Y),
                                    glm::vec3(),
                                    glm::vec3(),
                                    SUO_JIN,tab_w,max_h);
                                cursor_x += tab_w;  
                            } 
                        break;
                        case '\n':
                            demo->push_back_word_na(glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                HUI_CHE,6,max_h);
                            cursor_y += max_h;
                            cursor_x = 0;
                        break;
                        default:
                            {
                                auto cu = demo->getcu(*p);
                                if(!cu)
                                    break;
                                allIsSpace = false;
                                demo->push_back_word_na(glm::vec3(cursor_x,cursor_y,Word_Y),
                                    glm::vec3(0.0f,0.0f,0.0f),
                                    glm::vec3(1.0f,1.0f,1.0f),
                                    *p,cu->w,cu->h);
                                cursor_x += cu->w;
                                if(cu->h > max_h)
                                {
                                    max_h = cu->h;
                                }
                            }
                        break;
                    }
                    ++p;
                }
            }else{
                while(*p)
                {
                    switch(*p)
                    {
                        case ' ':
                            demo->insert_back_it_no(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                KONG_GE,WORD_W,max_h);
                            cursor_x += WORD_W;
                        break;
                        case '\t':
                            {   int tab_w = getTabW(cursor_x);
                                demo->insert_back_it_no(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                                    glm::vec3(),
                                    glm::vec3(),
                                    SUO_JIN,tab_w,max_h);
                                cursor_x += tab_w;
                            }  
                        break;
                        case '\n':
                            demo->insert_back_it_no(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(),
                                glm::vec3(),
                                HUI_CHE,6,max_h);
                            cursor_y += max_h;
                            cursor_x = 0;
                        break;
                        default:
                        {
                            auto cu = demo->getcu(*p);
                            if(!cu)
                                break;
                            demo->insert_back_it_no(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                                glm::vec3(0.0f,0.0f,0.0f),
                                glm::vec3(1.0f,1.0f,1.0f),
                                *p,cu->w,cu->h);
                            cursor_x += cu->w;
                            if(cu->h > max_h)
                            {
                                max_h = cu->h;
                            }
                            break;
                        }
                    }
                    ++p;
                }
                std::list<Word>::iterator last = *pit;
                std::list<Word>::iterator next = *pit;
                --last;
                demo->adjustPos(last,next);
            }
            demo->needUpdateColour = true;
            demo->needUpdatePlanes = true;
            demo->reDraw();
        }
    }
}

void Demo1::WindowResize(GLFWwindow*,int w,int h)
{
    glViewport(0,0,w,h);
    Window_width = w;
    Window_height = h;
    demo->reDraw();
}

void Demo1::MouseButtonCallBack(GLFWwindow* w,int btn,int isPressed,int v3)
{
    if (btn == 0)
    {
        if(isPressed)
        {
            if(!pit)
                pit = new std::list<Word>::iterator(demo->get_last_it());
            std::list<Word>::iterator last_it = *pit;
            double x,y;
            glfwGetCursorPos(w,&x,&y);
            demo->get_xy_it(*pit,x - WorldPosX,y - WorldPosY);
            if(*pit == demo->words.end())
            {
                *pit = last_it;
            }else{
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->reDraw();
                demo->reSetVernierClock();
            }
            //LeftButtonPressed = true;
            //
        }else{
            LeftButtonPressed = false;
        }
    }
    if(btn == 1)
    {
        if(isPressed)
        {
            RightButtonPressed = true;
            glfwGetCursorPos(w,&LastCursorX,&LastCursorY);
        }else{
            RightButtonPressed = false;
        }
    }
   
}

void Demo1::CursorCallBack(GLFWwindow* w,double x,double y)
{
    // if(LeftButtonPressed)
    // {
    //     double offsetx = x - LastCursorX;
    //     double offsety = y - LastCursorY;

    //     WorldAngleY -= PI_1_180() * offsetx;
    //     WorldAngleX += PI_1_180() * offsety;

    //     LastCursorX = x;
    //     LastCursorY = y;
    //     demo->reDraw();
    // }else 
    if(RightButtonPressed){
        double offsetx = x - LastCursorX;
        double offsety = y - LastCursorY;

        WorldPosY += offsety;
        WorldPosX += offsetx;

        LastCursorX = x;
        LastCursorY = y;
        demo->reDraw();
    }
    
}

void Demo1::WindowFocusCallBack(GLFWwindow* w,int f)
{
    if(f)
    {
        demo->hasFocus = true;
        demo->reSetVernierClock();
        demo->reDraw();
    }else{
        demo->hasFocus = false;
        demo->needDrawVernier = false;
        demo->reDraw();
    }
}

int main()
{
    Demo1 d;
    demo = &d;
    if( d.initWindow(Window_width,Window_height,"Demo1"))
    {
        printf("init window failed\n");
        return -1;
    }
    d.init();
    d.run();

    if(pit)
        delete pit;
}