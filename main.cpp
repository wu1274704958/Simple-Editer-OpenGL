
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

#define WORD_W 10
static int WORD_H = 18;

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

static const wchar_t const * KeyWord[] ={
    L"int",
    L"if",
    L"char",
    L"long",
    L"float",
    L"double",
    L"static",
    L"const",
    L"short",
    L"struct",
    L"return",
    L"for",
    L"while",
    L"break",
    L"continue",
    L"else",
    L"switch",
    L"case"
};
template<typename T>
struct ArrLen;

template<typename T,size_t N>
struct ArrLen<T [N]>{
    static const int value =  N;
};

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
                    WORD_H = cu.h;
                cus.push_back(cu);
            }
            fclose(file);
        }
        glfwSetCharCallback(m_window,Demo1::CharCallBack);
        glfwSetCharModsCallback(m_window,Demo1::CharModsCallBack);
        glfwSetKeyCallback(m_window,Demo1::KeyCallBack);
        glfwSetWindowSizeCallback(m_window,Demo1::WindowResize);
        glfwSetMouseButtonCallback(m_window,Demo1::MouseButtonCallBack);
        glfwSetCursorPosCallback(m_window,Demo1::CursorCallBack);

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
        frame_n = 0;
        needUpdateColour = true;
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
        frame_n = 0;
        return wor; 
    }
    void insert_back_it(std::list<Word>::iterator &it,glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
    {
        Word w(pos_,angle_,color_,c_,w_,h_);
        it = words.insert(it,w);
        std::list<Word>::iterator n_it = it;
        while(true)
        {
            n_it++;
            if(n_it == words.end())
            {
                break;
            }
            n_it->pos.x += w_;
            if(n_it->c == HUI_CHE)
            {
                break;
            }
        }
        it++;
        frame_n = 0;
        if(c_ != KONG_GE)
        {
            needUpdatePlanes = true;
            needUpdateColour = true;
        }
    }
    void insert_space_back_it(std::list<Word>::iterator &it,glm::vec3 &pos_,glm::vec3& angle_,glm::vec3& color_,wchar_t c_,int w_,int h_)
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
        frame_n = 0;
    }
    void remove_forword_it(std::list<Word>::iterator &it)
    {
        bool f = false;
        int x = it->pos.x;
        int y = it->pos.y;
        //bool f2 = false;
        //if(it->c == HUI_CHE)
        //{
        //    f2 = true;
        //}
        --it;
        int offsetx = it->pos.x - x;
        int offsety = it->pos.y - y;
        if(it->c == HUI_CHE)
            f = true;
        int ci = 0;
        it = words.erase(it);
        std::list<Word>::iterator n_it = it;
        while(true)
        {
            if(n_it == words.end())
            {
                break;
            }
            n_it->pos.x += offsetx;
            n_it->pos.y += offsety;
            
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
            n_it++;
        }
        frame_n = 0;
        //printf("%d",it->c);
        
        needUpdatePlanes = true;
        needUpdateColour = true;
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
protected:
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
            it->color = glm::vec3(1.0f,1.0f,1.0f);
            switch(it->c)
            {
                case L'/':
                {
                   Word *now = &(*it);
                   ++it;
                   END_BREAK(it)
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
                    for(int i = 0;i < ArrLen<decltype(KeyWord)>::value;++i)
                    {
                        f = true;
                        int len = wcslen(KeyWord[i]);
                        for(int j = 0;j < len;++j)
                        {
                            if(KeyWord[i][j] != it->c)
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
        if(frame_n <= 50)
            drawVernier(ortho_mat,world_mat);
        if(frame_n == 100)
            frame_n = 0;
        ++frame_n;
        // m_a += sinf(m_s) ;
        // if(m_a >= 360.0f)
        // {
        //     m_s = 0.0f;
        //     m_a = 0.0f;
        // }else{
        //     m_s += 0.01f;
        // }
        

        for(auto it = animate_list.begin();it != animate_list.end();)
        {
            if((*it)->angle.z >= 360.0f)
            {
                (*it)->angle.z = 0.0f;
                (*it)->angle.y = 0.0f;
                it = animate_list.erase(it);
                continue;
            }else{
                (*it)->angle.z += 10.0f;
                (*it)->angle.y += 10.0f;
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
        RenderDemo::destroy();
    }
    static void CharCallBack(GLFWwindow* w,unsigned int v);
    static void CharModsCallBack(GLFWwindow*,unsigned int,int);
    static void KeyCallBack(GLFWwindow*,int,int,int,int);
    static void WindowResize(GLFWwindow*,int,int);
    static void MouseButtonCallBack(GLFWwindow*,int,int,int);
    static void CursorCallBack(GLFWwindow*,double,double);
private:
    GLuint vertex_array,vertex_shader,fragment_shader,texture0,line_vs,line_fg,line_program,line_array;
    GLuint vertex_buffer,vernier_buffer,word_ebo;
    GLuint program;
    GLuint vposition,ucolor,ortho,world,model,tex0,line_ortho,line_world,line_model,line_color;
    int frame_n = 0;
    //float m_a = 0.0f;
    //float m_s = 0.0f;
    int char_map_w,char_map_h; 
    std::vector<Plane> planes;
    std::list<Word> words;
    std::vector<char_unit::CharUnit> cus;
    bool needUpdatePlanes = false;
    bool needUpdateColour = false;
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

void Demo1::KeyCallBack(GLFWwindow*,int v1,int v2,int v3,int v4)
{
    printf("Key %d %d %d %d\n",v1,v2,v3,v4);
    if(v3 == 1 || v3 == 2)
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
                    demo->insert_space_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
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
                if(pit == nullptr || *pit == demo->words.end())
                {
                     demo->push_back_word(glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            SUO_JIN,WORD_W * 4,max_h);
                }else{
                    demo->insert_back_it(*pit,glm::vec3(cursor_x,cursor_y,Word_Y),
                            glm::vec3(),
                            glm::vec3(),
                            SUO_JIN,WORD_W * 4,max_h);
                }
                cursor_x += WORD_W * 4;    
            break;
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
                    demo->frame_n = 0;
                }
            break;
            case 333:  // right
            
                if(demo->words.empty() || *pit == demo->words.end() || ( cursor_x > demo->words.back().pos.x && cursor_y == demo->words.back().pos.y ) )
                {
                    return;
                }
                ++(*pit);
                if(*pit == demo->words.end())
                {
                    --(*pit);
                    if((*pit)->c == HUI_CHE)
                    {
                        cursor_y += (*pit)->h;
                        cursor_x = 0;
                        demo->frame_n = 0;
                         ++(*pit);
                        return ;
                    }
                    cursor_x += (*pit)->w;
                    demo->frame_n = 0;
                    ++(*pit);
                    return ;
                }
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->frame_n = 0;
            break;
            case 328:  // up
                if(cursor_y == 0)
                    return;
                if(!pit)
                    pit = new std::list<Word>::iterator(demo->get_last_it());
                demo->get_less_xy_it(*pit,cursor_x,cursor_y); 
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->frame_n = 0;

            break;
            case 336:  // down
                if( demo->words.empty() || cursor_y == demo->words.back().pos.y)
                {
                    if(demo->words.back().c == HUI_CHE)
                    {
                        cursor_y += (*pit)->h;
                        *pit = demo->words.end();
                        cursor_x = 0;
                        demo->frame_n = 0;
                        return;
                    }
                    return;
                }
                    
                demo->get_more_xy_it(*pit,cursor_x,cursor_y); 
                if(*pit == demo->words.end())
                {
                    --(*pit);
                    cursor_x = (*pit)->pos.x + (*pit)->w;
                    cursor_y = (*pit)->pos.y;
                    demo->frame_n = 0;
                    ++(*pit);
                    return ;
                }
                cursor_x = (*pit)->pos.x;
                cursor_y = (*pit)->pos.y;
                demo->frame_n = 0;
            break;
        }
    }
}

void Demo1::WindowResize(GLFWwindow*,int w,int h)
{
    glViewport(0,0,w,h);
    Window_width = w;
    Window_height = h;
}

void Demo1::MouseButtonCallBack(GLFWwindow* w,int btn,int isPressed,int v3)
{
    if (btn == 0)
    {
        if(isPressed)
        {
            LeftButtonPressed = true;
            glfwGetCursorPos(w,&LastCursorX,&LastCursorY);
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
    if(LeftButtonPressed)
    {
        double offsetx = x - LastCursorX;
        double offsety = y - LastCursorY;

        WorldAngleY -= PI_1_180() * offsetx;
        WorldAngleX += PI_1_180() * offsety;

        LastCursorX = x;
        LastCursorY = y;
    }else if(RightButtonPressed){
        double offsetx = x - LastCursorX;
        double offsety = y - LastCursorY;

        WorldPosY += offsety;
        WorldPosX += offsetx;

        LastCursorX = x;
        LastCursorY = y;
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