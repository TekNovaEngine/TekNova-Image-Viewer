#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ImGuiTheme.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "tinyfiledialogs.h"

#include <iostream>

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

bool ExportImage(const char* filename, unsigned char* data, int width, int height, int channels);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += (float)yoffset; // Aggiungiamo l'offset della rotella del mouse
}

int WinMain()
{
    // Variabili
    int filteringMode = GL_LINEAR; // Inizializzato a GL_LINEAR come default

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Teknova Image Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "[INFO]: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontFromFileTTF("Content/Fonts/Roboto-Regular.ttf", 18);
    ImGui::StyleColorsDark();

    SetDarkThemeColors();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Register scroll callback
    glfwSetScrollCallback(window, scroll_callback);

    GLuint image_texture = 0;
    int image_width = 0, image_height = 0;
    float zoom = 1.0f;
    ImVec2 image_offset = ImVec2(0, 0);
    bool is_panning = false;
    ImVec2 pan_start = ImVec2(0, 0);
    ImVec2 current_pos;
    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events
        glfwPollEvents();

        // Adjust zoom level based on mouse wheel input
        if (io.MouseWheel != 0.0f)
        {
            zoom += io.MouseWheel * 0.1f;
            if (zoom < 0.1f) zoom = 0.1f;
            if (zoom > 10.0f) zoom = 10.0f;
            io.MouseWheel = 0.0f; // Reset the wheel value after handling it
        }

        // Handle panning
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            image_offset.x += delta.x;
            image_offset.y += delta.y;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DockSpace
        ImGui::DockSpaceOverViewport();

        // Image detail window
        ImGui::Begin("Dettagli immagine");
        if (ImGui::Button("Apri un'immagine"))
        {
            const char* file = tinyfd_openFileDialog("Apri immagine", "", 0, NULL, NULL, 0);
            if (file)
            {
                if (image_texture)
                {
                    glDeleteTextures(1, &image_texture);
                }
                unsigned char* data = stbi_load(file, &image_width, &image_height, NULL, 4);
                if (data)
                {
                    glGenTextures(1, &image_texture);
                    glBindTexture(GL_TEXTURE_2D, image_texture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringMode);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringMode);
                    stbi_image_free(data);
                }
                // Reset offset when loading new image
                image_offset = ImVec2(0, 0);
            }
        }

        if (image_texture)
        {
            ImGui::Text("Size: %dx%d", image_width, image_height);
            ImGui::SliderFloat("Zoom", &zoom, 0.1f, 10.0f);

            if (ImGui::CollapsingHeader("OpenGL Filtering"))
            {
                if (ImGui::RadioButton("Linear", filteringMode == GL_LINEAR))
                    filteringMode = GL_LINEAR;
                ImGui::SameLine();
                if (ImGui::RadioButton("Nearest", filteringMode == GL_NEAREST))
                    filteringMode = GL_NEAREST;
            }
            // Applica la modalità di filtering corrente alla texture
            glBindTexture(GL_TEXTURE_2D, image_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringMode);
        }
        if (image_texture)
        {
            if (ImGui::Button("Esporta Immagine"))
            {
                const char* filters[] = { "*.png", "*.jpg", "*.bmp", "*.tga" };
                const char* file = tinyfd_saveFileDialog("Esporta immagine", "Texture", 4, filters, NULL);
                if (file)
                {
                    // Estrai i dati dell'immagine dalla texture attualmente visualizzata
                    glBindTexture(GL_TEXTURE_2D, image_texture);
                    unsigned char* img_data = new unsigned char[image_width * image_height * 4];
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);

                    // Chiamata alla funzione ExportImage
                    bool success = ExportImage(file, img_data, image_width, image_height, 4); // 4 canali per RGBA

                    // Pulisci la memoria allocata per i dati dell'immagine
                    delete[] img_data;

                    // Controlla se l'esportazione è avvenuta con successo
                    if (success)
                    {
                        std::cout << "Immagine esportata con successo." << std::endl;
                    }
                    else
                    {
                        std::cout << "Errore durante l'esportazione dell'immagine." << std::endl;
                    }
                }
            }
        }

        ImGui::End();

        // Image viewing window
        ImGui::Begin("Viewport");
        if (image_texture)
        {
            ImVec2 available_size = ImGui::GetContentRegionAvail();
            float aspect_ratio = (float)image_width / (float)image_height;
            ImVec2 image_size = ImVec2(image_width * zoom, image_height * zoom);

            // Limit image size to the available region while considering zoom
            if (image_size.x > available_size.x || image_size.y > available_size.y)
            {
                if (available_size.x / aspect_ratio <= available_size.y)
                {
                    image_size.x = available_size.x * zoom;
                    image_size.y = available_size.x / aspect_ratio * zoom;
                }
                else
                {
                    image_size.x = available_size.y * aspect_ratio * zoom;
                    image_size.y = available_size.y * zoom;
                }
            }

            current_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(current_pos.x + image_offset.x, current_pos.y + image_offset.y));
            ImGui::Image((void*)(intptr_t)image_texture, image_size);
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    if (image_texture)
    {
        glDeleteTextures(1, &image_texture);
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

bool ExportImage(const char* filename, unsigned char* data, int width, int height, int channels)
{
    // Determina il formato del file in base all'estensione
    const char* extension = strrchr(filename, '.');
    if (!extension)
    {
        std::cout << "Errore: Estensione del file non riconosciuta." << std::endl;
        return false;
    }

    int format = 0;
    if (strcmp(extension, ".png") == 0)
    {
        format = stbi_write_png(filename, width, height, channels, data, width * channels);
    }
    else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0)
    {
        format = stbi_write_jpg(filename, width, height, channels, data, 90); // 90 è la qualità JPG
    }
    else if (strcmp(extension, ".bmp") == 0)
    {
        format = stbi_write_bmp(filename, width, height, channels, data);
    }
    else if (strcmp(extension, ".tga") == 0)
    {
        format = stbi_write_tga(filename, width, height, channels, data);
    }
    else
    {
        std::cout << "Errore: Formato del file non supportato." << std::endl;
        return false;
    }

    // Verifica se l'operazione di scrittura è stata eseguita con successo
    if (!format)
    {
        std::cout << "Errore durante l'esportazione dell'immagine." << std::endl;
        return false;
    }

    std::cout << "Immagine esportata con successo." << std::endl;
    return true;
}
