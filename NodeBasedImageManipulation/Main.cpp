#pragma once

#include "graph.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

using namespace std;

int main()
{
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Node Based Image Processor", nullptr, nullptr);
	if (window == nullptr)
		return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImNodes::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Make graph a singleton
    Graph graph;
    ImageBuffer* buffer = nullptr;

    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        {
            graph.Evaluate();
        }

        // 1. Node Canvas
        {
            ImGui::Begin("simple node editor");
            ImNodes::BeginNodeEditor();

            // Creates all nodes on the canvas
            graph.CreateNodesOnCanvas();
            graph.InitiateLinks();

            ImNodes::EndNodeEditor();
            {
                int from_channel, to_channel;
                if (ImNodes::IsLinkCreated(&from_channel, &to_channel))
                {
                    bool isConnected = graph.Connect(from_channel, to_channel);
                    if (!isConnected)
                    {
                        // Trigger error popup message
                    }
                }
            }

            {
                int link_id;
                if (ImNodes::IsLinkDestroyed(&link_id))
                {
                    graph.Disconnect(link_id);
                }
            }
            ImGui::End();
        }

        // 2. Node list window
        {
            ImGui::Begin("Available Nodes");

            ImGui::Text("Click on the buttons below to generate a node.");

            if (ImGui::Button("Input")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                InputNode* inputN = new InputNode(graph.GetNewId());
                graph.AddNode(inputN);
            }
            if (ImGui::Button("Output")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                OutputNode* inputN = new OutputNode(graph.GetNewId());
                graph.AddNode(inputN);
            }
            if (ImGui::Button("Brightness/Contrast")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                BrightnessContrastNode* inputN = new BrightnessContrastNode(graph.GetNewId());
                graph.AddNode(inputN);
            }
            if (ImGui::Button("Color Channel Splitter")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                ColorChannelSplitterNode* inputN = new ColorChannelSplitterNode(graph.GetNewId());
                graph.AddNode(inputN);
            }
            if (ImGui::Button("Blur")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                BlurNode* inputN = new BlurNode(graph.GetNewId());
                graph.AddNode(inputN);
            }
            if (ImGui::Button("Threshold")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                //ThresholdNode* inputN = new ThresholdNode(graph.GetNewId());
                //graph.AddNode(inputN);
            }
            ImGui::End();
        }

        // 3. This will become the property window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        int nSelNodes = ImNodes::NumSelectedNodes();
        vector<int> selectedNodeIds;
        if (nSelNodes)
        {
            selectedNodeIds.resize(ImNodes::NumSelectedNodes());
            ImNodes::GetSelectedNodes(&selectedNodeIds[0]);
        }

        int nSelLinks = ImNodes::NumSelectedLinks();
        vector<int> selectedLinkIds;
        if (nSelLinks)
        {
            selectedLinkIds.resize(ImNodes::NumSelectedLinks());
            ImNodes::GetSelectedLinks(&selectedLinkIds[0]);
        }

        // Either delete the selections or show an image
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            graph.DeleteNodes(selectedNodeIds);
            graph.DeleteLinks(selectedLinkIds);
        }
        else
        {
            // 4. Show an image
            if (nSelNodes)
            {
                Node* selectedNode = graph.GetNodeFromId(selectedNodeIds[0]);
                if (selectedNode)
                    buffer = selectedNode->GetImageBuffer();
            }
            else if (nSelLinks)
            {
                Link* selectedLink = graph.GetLinkFromId(selectedLinkIds[0]);
                if (selectedLink)
                    buffer = (ImageBuffer*)selectedLink->GetPropogatedData();
            }
            ImGui::Begin("Image Preview");

            if (buffer)
                buffer->ShowImage();

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImNodes::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
