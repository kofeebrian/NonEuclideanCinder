#include "SceneTunnel.h"

#include "cinder/CinderImGui.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/wrapper.h"

using namespace ci;

void SceneTunnel::setup(const std::unordered_map<std::string, DataSourceRef>& assets)
{
    // set GLFW
    glfwSetInputMode(mGlfwWindowRef, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // setup texture
    auto textureFormat = gl::Texture::Format().mipmap();
    mFloorTex = gl::Texture::create(loadImage(assets.at("checkerboard.png")), textureFormat);
    mTunnelTex = gl::Texture::create(loadImage(assets.at("rock-tunnel")), textureFormat);
    mSkyboxTex = gl::TextureCubeMap::create(loadImage(assets.at("galaxy-texture")),
        gl::TextureCubeMap::Format().mipmap());

    // setup glsl program
    auto textureGlsl = gl::getStockShader(gl::ShaderDef().texture());
    mColorGlsl = gl::getStockShader(gl::ShaderDef().color());
    auto skyboxGlsl = gl::GlslProg::create(assets.at("skybox.vert"), assets.at("skybox.frag"));

    // setup skybox
    mSkyboxBatch = gl::Batch::create(geom::Cube().size(vec3(300)), skyboxGlsl);
    mSkyboxBatch->getGlslProg()->uniform("uCubeMapTex", 0);

    // setup floor
    mFloorBatch = gl::Batch::create(geom::Plane().size(vec2(100)), textureGlsl);

    // setup tunnel
    mShortTunnel.setCount(6);
    mShortTunnel.setPosition(vec3(-10, 3, -5));
    mShortTunnel.setTexture(mTunnelTex);
    mShortTunnel.setupTunnel();
    mShortTunnel.setupSideWall();

    mLongTunnel.setCount(6);
    mLongTunnel.setPosition(vec3(10, 3, -5));
    mLongTunnel.setTexture(mTunnelTex);
    mLongTunnel.setupTunnel();
    mLongTunnel.setupFrontWall();

    ImGui::Initialize();

    // initialize camera properties
    mCam.setEyePoint({ 0, 3, 0 });
    mCam.lookAt({ 0, 1, 3 });
    mCam.toggleFloating();
}

void SceneTunnel::update(double currentTime)
{
    // Debug UI
    ImGui::Begin("Debug panel");
    ImGui::Text("Elapsed time:%.1f second", mlastTime);
    ImGui::End();

    ImGui::Begin("Key biding");
    ImGui::Text("W - Move forward");
    ImGui::Text("A - Move left");
    ImGui::Text("S - Move backward");
    ImGui::Text("D - Move right");
    ImGui::Text("Ctrl - Move downward");
    ImGui::Text("Space - Move upward");
    ImGui::Text("F - Freeze the camera");
    ImGui::Text("T - Toggle floating camera");
    ImGui::Text("G - Togle fullscreen mode");
    ImGui::Text("Esc - Close applicaiton");
    ImGui::End();

    // Update time logic
    mTimeOffset = currentTime - mlastTime;
    mlastTime = currentTime;

    // Poll for inputs
    processInput();
}

void SceneTunnel::draw()
{
    gl::clear(Color::gray(0.2f));
    gl::enableDepthWrite();
    gl::enableDepthRead();
    gl::setMatrices(mCam);

    // draw skybox
    mSkyboxTex->bind();
    mSkyboxBatch->draw();

    // draw floor
    mFloorTex->bind();
    mFloorBatch->draw();

    // draw new Tunnel
    mShortTunnel.draw();
    mLongTunnel.draw();
}

Camera* SceneTunnel::getCamera()
{
    return &mCam;
}

void SceneTunnel::handleKeyDown(KeyEvent event)
{
    if (event.getCode() == KeyEvent::KEY_f)
        mCam.toggleFreeze(mGlfwWindowRef);
    if (event.getCode() == KeyEvent::KEY_t)
        mCam.toggleFloating();

    // For testing
    if (event.getCode() == KeyEvent::KEY_i)
        mCam.move(MOVEMENT::FORWARD, mTimeOffset);
    if (event.getCode() == KeyEvent::KEY_k)
        mCam.move(MOVEMENT::BACKWARD, mTimeOffset);
    if (event.getCode() == KeyEvent::KEY_j)
        mCam.move(MOVEMENT::LEFT, mTimeOffset);
    if (event.getCode() == KeyEvent::KEY_l)
        mCam.move(MOVEMENT::RIGHT, mTimeOffset);
}

void SceneTunnel::handleMouseMove(MouseEvent event)
{
    if (firstMouseMove) {
        lastPos = event.getPos();
        firstMouseMove = false;
    } else {
        lastPos = currentPos;
    }

    currentPos = event.getPos();
    ivec2 offset = currentPos - lastPos;
    mCam.processMouse(offset.x, offset.y);
}

void SceneTunnel::processInput()
{
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_W) == GLFW_PRESS)
        mCam.move(MOVEMENT::FORWARD, mTimeOffset);
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_S) == GLFW_PRESS)
        mCam.move(MOVEMENT::BACKWARD, mTimeOffset);
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_A) == GLFW_PRESS)
        mCam.move(MOVEMENT::LEFT, mTimeOffset);
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_D) == GLFW_PRESS)
        mCam.move(MOVEMENT::RIGHT, mTimeOffset);
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_SPACE) == GLFW_PRESS)
        mCam.move(MOVEMENT::UPWARD, mTimeOffset);
    if (glfwGetKey(mGlfwWindowRef, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        mCam.move(MOVEMENT::DOWNWARD, mTimeOffset);
}
