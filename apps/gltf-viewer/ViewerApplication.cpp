#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/cameras.hpp"
#include "utils/gltf.hpp"
#include "utils/images.hpp"

#include <stb_image_write.h>
#include <tiny_gltf.h>

#include <windows.h>

void keyCallback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, 1);
  }
}

std::vector<GLuint> ViewerApplication::createBufferObjects(
    const tinygltf::Model &model){

  std::vector<GLuint> bufferObjects(model.buffers.size(), 0);

  glGenBuffers(GLsizei(model.buffers.size()), bufferObjects.data());
  for (size_t i = 0; i < model.buffers.size(); i++) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
    glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(), 0);
  }
  //Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  return bufferObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects(
    const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects,
    std::vector<VaoRange> &meshToVertexArrays)
{
  const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
  const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
  const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;

  std::vector<GLuint> vertexArrayObjects;

  meshToVertexArrays.resize(model.meshes.size());

  for (size_t meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
    const auto & currentMesh = model.meshes[meshIdx];

    auto &vaoRange = meshToVertexArrays[meshIdx];
    vaoRange.begin =
        GLsizei(vertexArrayObjects.size()); // Range for this mesh will be at
                                            // the end of vertexArrayObjects
    vaoRange.count =
        GLsizei(model.meshes[meshIdx].primitives.size()); // One VAO for each primitive

    vertexArrayObjects.resize(
        vertexArrayObjects.size() + currentMesh.primitives.size());

    glGenVertexArrays(vaoRange.count, &vertexArrayObjects[vaoRange.begin]);

    for (size_t primitiveIdx = 0; primitiveIdx < currentMesh.primitives.size();
         primitiveIdx++) {

      const auto vao = vertexArrayObjects[vaoRange.begin + primitiveIdx];
      const auto &primitive = currentMesh.primitives[primitiveIdx];
      glBindVertexArray(vao);

      {
        const auto iterator = primitive.attributes.find("POSITION");
        if (iterator != end(primitive.attributes)) { // If "POSITION" has been found in the map
          // (*iterator).first is the key "POSITION", (*iterator).second is
          // the value, ie. the index of the accessor for this attribute
          const auto accessorIdx = (*iterator).second;
          const auto &accessor =
              model.accessors[accessorIdx]; // get the correct
                                            // tinygltf::Accessor from
                                            // model.accessors
          const auto &bufferView =
              model.bufferViews[accessor.bufferView]; // get the correct tinygltf::BufferView from model.bufferViews.
          const auto bufferIdx = bufferView.buffer; // get the index of the buffer used by the bufferView

          const auto bufferObject = bufferObjects[bufferIdx]; // get the correct buffer object from the buffer index

          // Enable the vertex attrib array corresponding to
          // POSITION with glEnableVertexAttribArray
          glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
          assert(GL_ARRAY_BUFFER == bufferView.target);
          //Bind the buffer object to GL_ARRAY_BUFFER
          glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

          const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;
          // Compute the total byte offset using
          // the accessor and the buffer view

          glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, accessor.type,
              accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride),
              (const GLvoid *)byteOffset);
          // TODO Call glVertexAttribPointer with the correct arguments.
          // Remember size is obtained with accessor.type, type is obtained
          // with accessor.componentType. The stride is obtained in the
          // bufferView, normalized is always GL_FALSE, and pointer is the
          // byteOffset (don't forget the cast).
        }
      }

      {
        const auto iterator = primitive.attributes.find("NORMAL");
        if (iterator != end(primitive.attributes)) {
          const auto accessorIdx = (*iterator).second;
          const auto &accessor = model.accessors[accessorIdx];

          const auto &bufferView = model.bufferViews[accessor.bufferView];

          const auto bufferIdx = bufferView.buffer;

          const auto bufferObject = bufferObjects[bufferIdx];

          glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX);
          assert(GL_ARRAY_BUFFER == bufferView.target);
          glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

          const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;

          glVertexAttribPointer(VERTEX_ATTRIB_NORMAL_IDX, accessor.type,
              accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride),
              (const GLvoid *)(byteOffset));
        }
      }
      {
        const auto iterator = primitive.attributes.find("TEXCOORD_0");
        if (iterator != end(primitive.attributes)) {
          const auto accessorIdx = (*iterator).second;
          const auto &accessor = model.accessors[accessorIdx];

          const auto &bufferView = model.bufferViews[accessor.bufferView];

          const auto bufferIdx = bufferView.buffer;

          const auto bufferObject = bufferObjects[bufferIdx];

          glEnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD0_IDX);
          assert(GL_ARRAY_BUFFER == bufferView.target);
          glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

          const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;

          glVertexAttribPointer(VERTEX_ATTRIB_TEXCOORD0_IDX, accessor.type,
              accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride),
              (const GLvoid *)byteOffset);
        }
      }

      if (primitive.indices >= 0) {
        const auto accessorIdx = primitive.indices;
        const auto &accessor = model.accessors[accessorIdx];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto bufferIdx = bufferView.buffer;

        assert(GL_ELEMENT_ARRAY_BUFFER == bufferView.target);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
            bufferObjects[bufferIdx]); // Binding the index buffer to
                                       // GL_ELEMENT_ARRAY_BUFFER while
                                       // the VAO is bound is enough to
                                       // tell OpenGL we want to use
                                       // that index buffer for that VAO
      }
    }
  }

  glBindVertexArray(0);

  std::cout << "Size of the vertexArrayObjects: " << vertexArrayObjects.size()
            << std::endl;

  return vertexArrayObjects;
}
//TO BE VERIFIED
std::vector<GLuint> ViewerApplication::createTextureObjects(
    const tinygltf::Model &model) const {

    //Default sampler
    tinygltf::Sampler defaultSampler;
    defaultSampler.minFilter = GL_LINEAR;
    defaultSampler.magFilter = GL_LINEAR;
    defaultSampler.wrapS = GL_REPEAT;
    defaultSampler.wrapT = GL_REPEAT;
    defaultSampler.wrapR = GL_REPEAT;

    const int nbOfTextures = model.textures.size();
    std::vector<GLuint> textures = std::vector<GLuint>(nbOfTextures, 0);

    // Generate the textures objects
    glGenTextures(nbOfTextures, textures.data());

    for (int i = 0; i < model.textures.size(); i++) {

        // Assume a texture object has been created and bound to GL_TEXTURE_2D
        const auto &texture = model.textures[i]; // get i-th texture
        assert(texture.source >= 0);             // ensure a source image is present
        const auto &image = model.images[texture.source]; // get the image

        glBindTexture(
            GL_TEXTURE_2D, textures[i]); // Bind to target GL_TEXTURE_2D

        // fill the texture object with the data from the image
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
            GL_RGBA, image.pixel_type, image.image.data());
  
    const auto &sampler =
        texture.sampler >= 0 ? model.samplers[texture.sampler] : defaultSampler;
    // Set sampling parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        sampler.minFilter != -1 ? sampler.minFilter : defaultSampler.minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        sampler.magFilter != -1 ? sampler.magFilter : defaultSampler.magFilter);
    // Set wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, sampler.wrapR);

    if (sampler.minFilter == GL_NEAREST_MIPMAP_NEAREST ||
        sampler.minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        sampler.minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        sampler.minFilter == GL_LINEAR_MIPMAP_LINEAR) {
      glGenerateMipmap(GL_TEXTURE_2D);
    }

  }
    glBindTexture(GL_TEXTURE_2D, 0);
    return textures;
};

bool ViewerApplication::loadGltfFile(tinygltf::Model &model) {
  std::string path = m_gltfFilePath.string();

  tinygltf::TinyGLTF loader;

  std::string err, warn;

  std::cout << "Current path is " << fs::current_path() << '\n';

  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

  if (!err.empty())
    std::cerr << "Err: " << err << std::endl;
  if (!warn.empty())
    std::cerr << "Warn: " << warn << std::endl;

  if (!ret)
    std::cerr << "Load failure for " << path << std::endl;

  return ret;
}

int ViewerApplication::run()
{
  // Loader shaders
  const auto glslProgram =
      compileProgram({m_ShadersRootPath / m_AppName / m_vertexShader,
          m_ShadersRootPath / m_AppName / m_fragmentShader});

  const auto modelViewProjMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
  const auto modelViewMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
  const auto normalMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");
  // Light for diffuse_directional_light.fs.glsl
  const auto lightDirectionLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightDirection");
  const auto lightIntensityLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightIntensity");
  // Textures
  const auto baseColorTextureLocation =
      glGetUniformLocation(glslProgram.glId(), "uBaseColorTexture");
  

  tinygltf::Model model;

  if (!loadGltfFile(model))
    return -1;

  // Light init
  auto lightDirection = glm::vec3(1, 1, 1);
  auto lightIntensity = glm::vec3(1, 1, 1);
  bool lightIsFromCamera = false;

  //Texture load
  std::vector<GLuint> textureObjects = createTextureObjects(model);
  //Default white texture
  float white[] = {1,1,1,1};
  GLuint whiteTexture;
  // Generate the texture object
  glGenTextures(1, &whiteTexture);
  
  glBindTexture(GL_TEXTURE_2D, whiteTexture); // Bind to target GL_TEXTURE_2D
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white); // Set image data
  // Set sampling parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Set wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

  glBindTexture(GL_TEXTURE_2D, 0);


  auto &bufferObjects = createBufferObjects(model);

  std::vector<VaoRange> meshToVertexArrays;
  const auto vertexArrayObjects =
      createVertexArrayObjects(model, bufferObjects, meshToVertexArrays);

  glm::vec3 bboxMin, bboxMax;
  computeSceneBounds(model, bboxMin, bboxMax);
  const auto bboxDiag = bboxMax - bboxMin;

  // For the projection matrix and camera init
  auto maxDistance = glm::length(bboxDiag);
  if (maxDistance <= 0.f)
    maxDistance = 100.f;
  TrackballCameraController cameraController{
      m_GLFWHandle.window(), 0.5f * maxDistance};

  if (m_hasUserCamera) {
    std::cout << "Using user camera " << std::endl;
    cameraController.setCamera(m_userCamera);
  } else {
    const auto upVec = glm::vec3(0, 1, 0);
    const auto bboxCenter = (bboxMax + bboxMin) * 0.5f;
    const auto eye = bboxDiag.z > 0
                         ? bboxCenter + bboxDiag
                         : bboxCenter + 2.0f * glm::cross(bboxDiag, upVec);

    cameraController.setCamera(Camera{eye, bboxCenter, upVec});

    std::cout << "Diag: " << bboxDiag << std::endl;
  }

  const auto projMatrix =
      glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,
          0.001f * maxDistance, 1.5f * maxDistance);

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);
  glslProgram.use();


  const auto bindMaterial = [&](const auto materialIndex) {
    if (materialIndex >= 0) {

      const auto &material = model.materials[materialIndex];
      const auto &pbrMetallicRoughness = material.pbrMetallicRoughness;
      if (pbrMetallicRoughness.baseColorTexture.index >= 0) {
        const tinygltf::Texture &texture =
            model.textures[pbrMetallicRoughness.baseColorTexture.index];
        glActiveTexture(GL_TEXTURE0);
        const GLuint texId = textureObjects[texture.source];
        glBindTexture(GL_TEXTURE_2D, texId);
        // By setting the uniform to 0, we tell OpenGL the texture is bound on
        // tex unit 0:
        glUniform1i(baseColorTextureLocation, 0);
      } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        // By setting the uniform to 0, we tell OpenGL the texture is bound on
        // tex unit 0:
        glUniform1i(baseColorTextureLocation, 0);
      }

    } else {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, whiteTexture);
      // By setting the uniform to 0, we tell OpenGL the texture is bound on tex
      // unit 0:
      glUniform1i(baseColorTextureLocation, 0);
    }
  };


  // Lambda function to draw the scene
  const auto drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto viewMatrix = camera.getViewMatrix();

    // The light is constant for 1 draw

    if (lightDirectionLocation >= 0) {

      if (lightIsFromCamera) // From the camera
        glUniform3f(lightDirectionLocation, 0, 0,
            1); // Don't change the lightDirection value
      else {
        const auto lightDirectionInViewSpace = glm::normalize(
            glm::vec3(viewMatrix * glm::vec4(lightDirection, 0.)));

        glUniform3f(lightDirectionLocation, lightDirectionInViewSpace[0],
            lightDirectionInViewSpace[1], lightDirectionInViewSpace[2]);
      }
    }

    if (lightIntensityLocation >= 0)
      glUniform3f(lightIntensityLocation, lightIntensity[0], lightIntensity[1],
          lightIntensity[2]);

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          const tinygltf::Node node = model.nodes[nodeIdx];
          glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);

          if (node.mesh >= 0) {
            const auto mvMatrix = viewMatrix * modelMatrix;
            const auto mvpMatrix = projMatrix * mvMatrix;

            const auto normalMatrix = glm::transpose(glm::inverse(mvMatrix));

            const auto &mesh = model.meshes[node.mesh];
            const auto &vaoRange = meshToVertexArrays[node.mesh];

            glUniformMatrix4fv(modelViewProjMatrixLocation, 1, GL_FALSE,
                glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(
                modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvMatrix));
            glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE,
                glm::value_ptr(normalMatrix));

            for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {

              const auto vao = vertexArrayObjects[vaoRange.begin + pIdx];
              const auto &primitive = mesh.primitives[pIdx];


              bindMaterial(primitive.material);
              glBindVertexArray(vao);
              if (primitive.indices >= 0) {
                const auto &accessor = model.accessors[primitive.indices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto byteOffset =
                    accessor.byteOffset + bufferView.byteOffset;
                glDrawElements(primitive.mode, GLsizei(accessor.count),
                    accessor.componentType, (const GLvoid *)byteOffset);
              } else {
                // Take first accessor to get the count
                const auto accessorIdx = (*begin(primitive.attributes)).second;
                const auto &accessor = model.accessors[accessorIdx];
                glDrawArrays(primitive.mode, 0, GLsizei(accessor.count));
              }
            }
          }
        };

    // Draw the scene referenced by gltf file
    if (model.defaultScene >= 0) {
      auto &sceneNodes = model.scenes[model.defaultScene].nodes;
      for (int i = 0; i < sceneNodes.size(); i++) { //TODO: use iterator instead
        drawNode(sceneNodes[i], glm::mat4(1));
      }
    }
  };


  //RENDER
  if (!m_OutputPath.empty()){
    std::vector<unsigned char> pixels( 3L * m_nWindowWidth * m_nWindowHeight);
    renderToImage(m_nWindowWidth, m_nWindowHeight, 3, pixels.data(),
        [&]() { drawScene(cameraController.getCamera()); });

    flipImageYAxis(m_nWindowWidth, m_nWindowHeight, 3, pixels.data()); //OpenGL data is different from png
    const auto strPath = m_OutputPath.string();
    stbi_write_png(
        strPath.c_str(), m_nWindowWidth, m_nWindowHeight, 3, pixels.data(), 0);
    std::cout << "Png file generated: " << m_OutputPath << std::endl;
    return 0;
  }

  // LOOP (input + update + render)
  // Loop until the user closes the window
  for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose();
       ++iterationCount) {
    const auto seconds = glfwGetTime();

    const auto camera = cameraController.getCamera();
    drawScene(camera);

    // GUI code:
    imguiNewFrame();

    {
      ImGui::Begin("GUI");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
          1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("eye: %.3f %.3f %.3f", camera.eye().x, camera.eye().y,
            camera.eye().z);
        ImGui::Text("center: %.3f %.3f %.3f", camera.center().x,
            camera.center().y, camera.center().z);
        ImGui::Text(
            "up: %.3f %.3f %.3f", camera.up().x, camera.up().y, camera.up().z);

        ImGui::Text("front: %.3f %.3f %.3f", camera.front().x, camera.front().y,
            camera.front().z);
        ImGui::Text("left: %.3f %.3f %.3f", camera.left().x, camera.left().y,
            camera.left().z);

        if (ImGui::Button("CLI camera args to clipboard")) {
          std::stringstream ss;
          ss << "--lookat " << camera.eye().x << "," << camera.eye().y << ","
             << camera.eye().z << "," << camera.center().x << ","
             << camera.center().y << "," << camera.center().z << ","
             << camera.up().x << "," << camera.up().y << "," << camera.up().z;
          const auto str = ss.str();
          glfwSetClipboardString(m_GLFWHandle.window(), str.c_str());
        }
      } // Camera

      if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float lightTheta = 0.f, lightPhi = 0.f;

        if (ImGui::SliderFloat("theta", &lightTheta, 0, glm::pi<float>()) ||
            ImGui::SliderFloat("phi", &lightPhi, 0, 2.f * glm::pi<float>())) {
          const auto sinTheta = glm::sin(lightTheta);
          const auto cosTheta = glm::cos(lightTheta);
          const auto sinPhi = glm::sin(lightPhi);
          const auto cosPhi = glm::cos(lightPhi);
          lightDirection =
              glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
        }

        static glm::vec3 lightColor(1.f, 1.f, 1.f);
        static float lightIntensityFactor = 1.f;

        if (ImGui::ColorEdit3("color", (float *)&lightColor) ||
            ImGui::InputFloat("intensity", &lightIntensityFactor)) {
          lightIntensity = lightColor * lightIntensityFactor;
        }
      }
      ImGui::Checkbox("Put the light on the camera", &lightIsFromCamera);

      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    auto ellapsedTime = glfwGetTime() - seconds;
    auto guiHasFocus =
        ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) {
      cameraController.update(float(ellapsedTime));
    }

    m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    Sleep(10);
  }

  // TODO clean up allocated GL data

  return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width,
    uint32_t height, const fs::path &gltfFile,
    const std::vector<float> &lookatArgs, const std::string &vertexShader,
    const std::string &fragmentShader, const fs::path &output) :
    m_nWindowWidth(width),
    m_nWindowHeight(height),
    m_AppPath{appPath},
    m_AppName{m_AppPath.stem().string()},
    m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
    m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
    m_gltfFilePath{gltfFile},
    m_OutputPath{output}
{
  if (!lookatArgs.empty()) {
    m_hasUserCamera = true;
    m_userCamera =
        Camera{glm::vec3(lookatArgs[0], lookatArgs[1], lookatArgs[2]),
            glm::vec3(lookatArgs[3], lookatArgs[4], lookatArgs[5]),
            glm::vec3(lookatArgs[6], lookatArgs[7], lookatArgs[8])};
  }

  if (!vertexShader.empty()) {
    m_vertexShader = vertexShader;
  }

  if (!fragmentShader.empty()) {
    m_fragmentShader = fragmentShader;
  }

  ImGui::GetIO().IniFilename =
      m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows
                                  // positions in this file

  glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);

  printGLVersion();
}