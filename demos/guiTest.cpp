#include <iostream>
#include <cmath>

#define GLFW_INCLUDE_NONE // < so we can use the default glalt path here...

#include <gl_hpp/glalt/gl4.1.h>
#include <gl_hpp/glalt/glext.h>
#include <gl_hpp/opengl.hpp>
#include <GLFW/glfw3.h>

#include <Virtuoso/GL/GLFWApplication.h>

#define ImguiRender_h_IMPLEMENTATION
#define VIRTUOSO_TEXTURELOADER_IMPLEMENTATION
#define VIRTUOSO_SHADERPROGRAMLIB_IMPLEMENTATION
#include <Virtuoso/GL/ImGUIRenderer.h>
#undef ImguiRender_h_IMPLEMENTATION
#undef VIRTUOSO_TEXTURELOADER_IMPLEMENTATION
#undef VIRTUOSO_SHADERPROGRAMLIB_IMPLEMENTATION

#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_glfw.cpp>

#include "../IMGUIQuakeConsole.h"

// push glsl state
// multiline comment
// wishlist
// autocomplete contexts (eg for glsl)
// regex filters
// allow processing of rule groups for output - eg. int <identifier> lets us color the identifer separately

const std::string glsl_qualifiers[] =
{
"const", "in", "inout", "out", "smooth", "flat", "noperspective", "invariant", "centroid",
    "coherent", "volatile", "restrict", "readonly", "writeonly", "uniform", "buffer", "shared", "sampler", "patch", "binding", "offset", "highp", "mediump", "lowp", "precise",
};

const std::string glsl_keywords[] = {"subroutine", "return", "break", "if", "for", "while", "do", "discard", "continue", "struct", "switch"};

const std::string glsl_types[] =
{
    "void",
    "bool",
    "int",
    "uint",
    "float",
    "double",
    "ivec2", "ivec3", "ivec4",
    "uvec2","uvec3","uvec4",
    "vec2", "vec3","vec4",
    "dvec2","dvec3","dvec4",
    "bvec2", "bvec3", "bvec4",
    "sampler1D","sampler2D", "sampler3D",
    "image1D", "image2D", "image3D",
    "mat2", "mat3", "mat4",
    "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4",
    "mat4x2", "mat4x3", "mat4x4",
    "dmat2", "dmat3", "dmat4",
    "dmat2x2", "dmat2x3", "dmat2x4",
    "dmat3x2", "dmat3x3", "dmat3x4",
    "dmat4x2", "dmat4x3", "dmat4x4",
    "samplerCube", "imageCube",
    "sampler2DRect", "image2DRect",
    "sampler2DArray", "sampler1DArray", "image1DArray", "image2DArray",
    "samplerBuffer", "imageBuffer",
    "sampler2DMS", "image2DMS", "sampler2DMSArray", "image2DMSArray",
    "samplerCubeArray", "imageCubeArray", "sampler1DShadow", "sampler2DShadow", "sampler2DRectShadow", "sampler1DArrayShadow", "sampler2DArrayShadow", "samplerCubeShadow",
    "samplerCubeArrayShadow",
    "isampler1D","isampler2D", "isampler3D",
    "iimage1D", "iimage2D", "iimage3D",
     "isamplerCube", "iimageCube", "isampler2DRect",
    "iimage2DRect", "isampler1DArray","isampler2DArray", "iimage1DArray", "iimage2DArray","isamplerBuffer", "iimageBuffer", "isampler2DMS", "iimage2DMS", "isampler2DMSArray", "iimage2DMSArray", "isamplerCubeArray", "iimageCubeArray",
    "atomic_uint", "usampler1D","usampler2D", "usampler3D", "uimage1D", "uimage2D", "uimage3D",
    "usamplerCube", "uimageCube", "usampler2DRect", "uimage2DRect", "usampler1DArray","usampler2DArray", "uimage1DArray", "uimage2DArray",
    "usamplerBuffer", "uimageBuffer", "usampler2DMS",
    "uimage2DMS",
    "usampler2DMSArray",
    "uimage2DMSArray", "usamplerCubeArray", "uimageCubeArray"
    
    // left off at atomic uint
};

// other keywords

const std::string glsl_functions[] =
{
    "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "abs", "sign",
    "floor", "trunc", "round", "roundEven", "ceil", "fract", "mod", "min", "max", "clamp", "mix", "step",
    "smoothstep", "isnan", "isinf", "floatBitsToUint", "floatBitsToInt", "intBitsToFloat", "fma", "frexp",
    "Idexp", "packUnorm2x16", "packSnorm2x16", "unpackUnorm2x16", "unpackSnorm2x16", "unpackUnorm4x8", "unpackSnorm4x8", "packDouble2x32", "unpackDouble2x32", "packHalf2x16", "unpackHalf2x16",
    "length", "distance", "dot", "cross", "normalize", "faceforward", "reflect", "refract", "matrixCompMult", "outerProduct", "transpose", "inverse", "determinant",
    "lessThan", "greaterThan", "lessThanEqual", "greaterThanEqual", "equal", "notEqual", "any", "all", "not", "uaddCarry", "usubBorrow", "umulExtended", "imulExtended", "bitfieldExtract", "bitfieldReverse", "bitfieldInsert", "bitCount", "findLSB", "findMSB", "atomicCounterIncrement",
    "atomicCounterDecrement", "atomicCounter", "atomicCounterOp", "atomicCounterCompSwap", "atomicCounterCompSwap", "atomicOP", "imageSize", "imageSamples", "imageLoad", "imageStore",
    "imageAtomicAdd", "imageAtomicMin", "imageAtomicMax", "imageAtomicAnd", "imageAtomicOr", "imageAtomicXor", "imageAtomicExchange", "imageAtomicCompSwap", "dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth", "fwidthFine", "fwidthCoarse", "interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset", "noise1", "noisen",
    "EmitStreamVertex", "EndStreamPrimitive", "EndPrimitive", "EmitVertex", "barrier", "memoryBarrier",
    "groupMemoryBarrier", "memoryBarrierAtomicCounter", "memoryBarrierShared", "memoryBarrierBuffer",
    "memoryBarrierImage", "allInvocationsEqual", "allInvocation", "textureSize", "textureQueryLod", "textureQueryLevels", "textureSamples", "texture", "textureLod", "textureProj", "textureOffset",
    "texelFetch", "texelFetchOffset", "textureProjOffset", "textureLodOffset", "textureProjLod", "textureProjLodOffset", "textureGrad", "textureGradOffset", "textureProjGrad", "textureProjGradOffset","textureGather", "textureGatherOffset", "textureGatherOffsets"
    
};

// built in variables are gl_*

//const int NUM_KEYWORDS = sizeof(glsl_types) / sizeof(std::string);


struct ConsoleApplication : public GLFWApplication
{
    IMGUIQuakeConsole console;

    ConsoleApplication()
    {
        console << "-- Message from ConsoleApplication() constructor" << std::endl;
        console << "[warning] - This thing supports color!\n";
        
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        ImGui_ImplGlfw_Init(window, true, GlfwClientApi_OpenGL);
        
        console.bindMemberCommand("glslTest", *this, &ConsoleApplication::doGLSLTest);
    }
    
    void render()
    {
        static ImguiRenderState imguiRenderer;
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        ImGui::NewFrame();

        bool open;
        console.Draw("Quake Style Console Demo", &open);
        
        ImGui::EndFrame();
        ImGui::Render();
        imguiRenderer.renderGUI(ImGui::GetDrawData());
    }
    
    /*std::string makeKeywordsRegexStr()
    {
        std::stringstream sstr;
        
        sstr << "\\b" << glsl_keywords[0] << "\\b";
        for (int i = 1 ; i < NUM_KEYWORDS; i++)
        {
            sstr << '|' << "\\b"<< glsl_keywords[i] <<"\\b";
        }
        
        return (sstr.str());
    }*/
    
    void doGLSLTest()
    {
        const std::string& printSrc = R"STRING(

# this is meant to show off some functionality in the console.
<glsl>

        precision highp float;

        in vec2 coords;
        in vec4 color;
        out vec4 col;

        uniform sampler2D tex;

        /***
            This is a multiline comment
        ***/

        void main(void)
        {
        // this just sets the color!
            col = color * texture(tex, coords).r;
        }

</glsl>

        )STRING";
        
        console << printSrc;
    }
};


int main(void)
{
    if (!glfwInit())
        return -1;
    
    glfwSetErrorCallback(GLFWApplication::error_callback);

    ConsoleApplication app;
    
    if (!app)
    {
        glfwTerminate();
        return -1;
    }
    
    app.mainLoop();
   
    glfwTerminate();
    
    return 0;
}