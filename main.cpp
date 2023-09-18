#include <algorithm>
#include <imgui.h>
#include <imnodes.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <set>
#include <functional>
#define GL_SILENCE_DEPRECATION

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include <GLFW/glfw3.h>

struct Node
{
    int id;
    std::string type;
    char letter[2];
};

std::vector<Node> nodes;
std::vector<int> links;

bool KernelPathIsValid()
{
    auto startNodeIt = std::find_if(nodes.begin(), nodes.end(), [](const Node &node)
                                    { return node.type == "kernel_start"; });

    if (startNodeIt == nodes.end())
    {
        std::cout << "Pas trouvé de kernel_start\n";
        return false;
    }

    std::set<int> visitedNodes;

    std::function<bool(int)> checkPath = [&](int currentNodeId)
    {
        visitedNodes.insert(currentNodeId);

        for (int i = 0; i < links.size(); i += 2)
        {
            if (links[i] == currentNodeId && visitedNodes.find(links[i + 1]) == visitedNodes.end())
            {
                auto nodeIt = std::find_if(nodes.begin(), nodes.end(), [&](const Node &node)
                                           { return node.id == links[i + 1]; });

                if (nodeIt != nodes.end())
                {
                    if (nodeIt->type == "kernel_end")
                    {
                        return true;
                    }

                    if (checkPath(nodeIt->id))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    };

    return checkPath(startNodeIt->id);
}

void SaveNodesToAssembler()
{
    if (!KernelPathIsValid())
    {
        std::cout << "Le chemin du kernel n'est pas valide\n";
        return;
    }

    std::ofstream outFile("kernel.asm");

    auto startNodeIt = std::find_if(nodes.begin(), nodes.end(), [](const Node &node)
                                    { return node.type == "kernel_start"; });

    if (startNodeIt == nodes.end())
    {
        std::cout << "Pas trouvé de kernel_start\n";
        return;
    }

    outFile << "org 0x7C00\n";
    outFile << "bits 16\n";

    std::set<int> visitedNodes;

    std::function<void(int)> writeInstructions = [&](int currentNodeId)
    {
        visitedNodes.insert(currentNodeId);

        for (int i = 0; i < links.size(); i += 2)
        {
            if (links[i] == currentNodeId && visitedNodes.find(links[i + 1]) == visitedNodes.end())
            {
                auto nodeIt = std::find_if(nodes.begin(), nodes.end(), [&](const Node &node)
                                           { return node.id == links[i + 1]; });

                if (nodeIt != nodes.end())
                {
                    if (nodeIt->type == "print_char")
                    {
                        outFile << "mov ah, 0x0e\n";
                        outFile << "mov al, '" << nodeIt->letter[0] << "'\n";
                        outFile << "int 0x10\n";
                    }
                    else if (nodeIt->type == "kernel_end")
                    {
                        outFile << "times 510-($-$$) db 0\n";
                        outFile << "dw 0AA55h\n";
                    }

                    writeInstructions(nodeIt->id);
                }
            }
        }
    };

    writeInstructions(startNodeIt->id);

    outFile.close();
    std::cout << "Nodes enregistrés dans 'kernel.asm'\n";
}

int main()
{
    glfwSetErrorCallback([](int error, const char *description)
                         { fprintf(stderr, "Glfw Error %d: %s\n", error, description); });
    if (!glfwInit())
        return 1;

    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "No-code Kernel Creator", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImNodes::CreateContext();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Fichier"))
            {
                if (ImGui::MenuItem("Enregistrer"))
                {
                    SaveNodesToAssembler();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImNodes::BeginNodeEditor();

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Ajouter kernel_start"))
            {
                nodes.push_back({static_cast<int>(nodes.size()), "kernel_start"});
            }
            if (ImGui::MenuItem("Ajouter kernel_end"))
            {
                nodes.push_back({static_cast<int>(nodes.size()), "kernel_end"});
            }
            if (ImGui::MenuItem("Ajouter print_char"))
            {
                Node new_node;
                new_node.id = static_cast<int>(nodes.size());
                new_node.type = "print_char";
                new_node.letter[0] = 'A';
                new_node.letter[1] = '\0';
                nodes.push_back(new_node);
            }

            ImGui::EndPopup();
        }

        for (auto node_it = nodes.begin(); node_it != nodes.end();)
        {
            ImNodes::BeginNode(node_it->id);

            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node_it->type.c_str());
            ImNodes::EndNodeTitleBar();

            if (node_it->type == "kernel_start")
            {
                ImNodes::BeginOutputAttribute(node_it->id);
                ImGui::Text("Start");
                ImNodes::EndOutputAttribute();
            }
            else if (node_it->type == "kernel_end")
            {
                ImNodes::BeginInputAttribute(node_it->id);
                ImGui::Text("End");
                ImNodes::EndInputAttribute();
            }
            else if (node_it->type == "print_char")
            {
                ImNodes::BeginInputAttribute(node_it->id);
                ImGui::Text("Input");
                ImNodes::EndInputAttribute();

                ImNodes::BeginOutputAttribute(node_it->id + 10000);
                ImGui::Text("Output");
                ImNodes::EndOutputAttribute();

                ImGui::InputText("letter", node_it->letter, 2);
            }

            if (ImGui::BeginPopupContextItem("NodeContext"))
            {
                if (ImGui::MenuItem("Supprimer"))
                {
                    node_it = nodes.erase(node_it);
                    continue;
                }
                ImGui::EndPopup();
            }

            ImNodes::EndNode();

            ++node_it;
        }

        for (int i = 0; i < links.size(); i += 2)
        {
            ImNodes::Link(i, links[i], links[i + 1]);
        }

        ImNodes::EndNodeEditor();

        int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
        {
            links.push_back(start_attr);
            links.push_back(end_attr);
        }

        int link_id;
        while (ImNodes::IsLinkDestroyed(&link_id))
        {
            auto it = std::find(links.begin(), links.end(), link_id);
            if (it != links.end())
            {
                links.erase(it, it + 2);
            }
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImNodes::EndNodeEditor();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImNodes::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}