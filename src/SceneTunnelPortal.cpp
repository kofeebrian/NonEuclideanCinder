#include "SceneTunnelPortal.h"

#include "cinder/CinderImGui.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/wrapper.h"

using namespace ci;

void SceneTunnelPortal::setup(const std::unordered_map<std::string, DataSourceRef>& assets)
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
    auto skybox = geom::Cube().size(vec3(300));
    mSkyboxBatch = gl::Batch::create(skybox, skyboxGlsl);
    mSkyboxBatch->getGlslProg()->uniform("uCubeMapTex", 0);

    // setup floor
    auto floor = geom::Plane().size(vec2(100));
    mFloorBatch = gl::Batch::create(floor, textureGlsl);
    auto floor2 = geom::Plane().size(vec2(100, 300)).origin(vec3(0, -30.001f, 0));
    mFloor2Batch = gl::Batch::create(floor2, textureGlsl);
    auto floor3 = geom::Plane().size(vec2(100, 100.0 / 3.0)).origin(vec3(0, -100.0f, 0));
    mFloor3Batch = gl::Batch::create(floor3, textureGlsl);

    // setup tunnel
    mShortTunnel.setCount(1);
    mShortTunnel.setPosition(vec3(-10, 3, -5));
    mShortTunnel.setTexture(mTunnelTex);
    mShortTunnel.setupTunnel();

    mLongTunnel.setCount(6);
    mLongTunnel.setPosition(vec3(10, 3, -5));
    mLongTunnel.setTexture(mTunnelTex);
    mLongTunnel.setupTunnel();

    // setup illution tunnel
    mImgShortTunnel.setCount(6);
    mImgShortTunnel.setPosition(vec3(-10, -27, -5));
    mImgShortTunnel.setTexture(mTunnelTex);
    mImgShortTunnel.setupTunnel();

    mImgLongTunnel.setCount(1);
    mImgLongTunnel.setPosition(vec3(10, -97, -5));
    mImgLongTunnel.setTexture(mTunnelTex);
    mImgLongTunnel.setupTunnel();

    // setup portals of short tunnels
    mPortals.emplace_back(mCam, mShortTunnel.mPosition, Portal::Z);
    mPortals.emplace_back(mCam, mImgShortTunnel.mPosition, Portal::NEG_Z);
    mPortals.emplace_back(
        mCam,
        mImgShortTunnel.mPosition + (float)mImgShortTunnel.mCount * vec3(0, 0, -mImgShortTunnel.mLong),
        Portal::Z);
    mPortals.emplace_back(
        mCam,
        mShortTunnel.mPosition + (float)mShortTunnel.mCount * vec3(0, 0, -mShortTunnel.mLong),
        Portal::NEG_Z);

    // setup portal of long tunnels
    mPortals.emplace_back(mCam, mLongTunnel.mPosition, Portal::Z);
    mPortals.emplace_back(mCam, mImgLongTunnel.mPosition, Portal::NEG_Z);
    mPortals.emplace_back(
        mCam,
        mImgLongTunnel.mPosition + (float)mImgLongTunnel.mCount * vec3(0, 0, -mImgLongTunnel.mLong),
        Portal::Z);
    mPortals.emplace_back(
        mCam,
        mLongTunnel.mPosition + (float)mLongTunnel.mCount * vec3(0, 0, -mLongTunnel.mLong),
        Portal::NEG_Z);

    for (int i = 0; i < mPortals.size() - 1; i += 2) {
        mPortals[i].setLinkedPortal(mPortals[i + 1]);
        mPortals[i + 1].setLinkedPortal(mPortals[i]);
    }
    // mPortals[0].setLinkedPortal(mPortals[1]);
    // mPortals[1].setLinkedPortal(mPortals[0]);
    // mPortals[2].setLinkedPortal(mPortals[3]);
    // mPortals[3].setLinkedPortal(mPortals[2]);
    for (auto& portal : mPortals) {
        portal.setSize(vec2(6, 4.5));
        portal.setup();
    }
    ImGui::Initialize();

    // initialize camera properties
    mCam.setEyePoint({ 0, 3, 0 });
    mCam.lookAt({ 0, 1, 3 });
    mCam.toggleFloating();
}

void SceneTunnelPortal::update(double currentTime)
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

    // Portal update
    for (auto& portal : mPortals)
        portal.update();

    for (auto& portal : mPortals) {
        if (portal.isIntersect(mLastCamPos, mCam.getEyePoint())) {
            portal.warp(mCam);
            break;
        }
    }

    // Update Last Position
    mLastCamPos = mCam.getEyePoint();
    // Poll for inputs
    processInput();
}

void SceneTunnelPortal::draw()
{
    gl::clear(Color::gray(0.2f));
    gl::enableDepthWrite();
    gl::enableDepthRead();
    gl::setMatrices(mCam);

    gl::enableStencilTest();
    for (auto& portal : mPortals) {
        gl::colorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        gl::depthMask(GL_FALSE);
        gl::stencilFunc(GL_NEVER, 0, 0xFF);
        gl::stencilOp(GL_INCR, GL_KEEP, GL_KEEP);
        gl::clear(GL_STENCIL_BUFFER_BIT);

        portal.draw();

        gl::colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl::depthMask(GL_TRUE);
        gl::stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        gl::stencilFunc(GL_LEQUAL, 1, 0xFF);
        gl::pushViewMatrix();

        mat4 newView = Portal::getNewViewMatrix(mCam.getViewMatrix(), portal.getModelMatrix(), portal.getLinkedPortal()->getModelMatrix());
        gl::setViewMatrix(newView);
        drawSceneObjects();
        gl::popViewMatrix();
    }
    gl::disableStencilTest();

    gl::clear(GL_DEPTH_BUFFER_BIT);
    gl::colorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gl::setMatrices(mCam);
    for (auto& portal : mPortals)
        portal.draw();
    gl::colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawSceneObjects();
}

void SceneTunnelPortal::drawSceneObjects()
{
    // draw skybox
    mSkyboxTex->bind();
    mSkyboxBatch->draw();

    // draw floor
    mFloorTex->bind();
    mFloorBatch->draw();
    mFloor2Batch->draw();
    mFloor3Batch->draw();

    // draw new Tunnel
    mShortTunnel.draw();
    mLongTunnel.draw();

    // draw iluution scene
    mImgShortTunnel.draw();
    mImgLongTunnel.draw();
}

void SceneTunnelPortal::drawPortalImages()
{
    gl::enableStencilTest();
    gl::colorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gl::depthMask(GL_FALSE);
    gl::stencilFunc(GL_NEVER, 0, 0xFF);
    gl::stencilOp(GL_INCR, GL_KEEP, GL_KEEP);
    gl::clear(GL_STENCIL_BUFFER_BIT);
    mPortals[0].draw();
    for (int i = 1; i < mPortals.size() - 1; i++) {
        gl::stencilFunc(GL_EQUAL, 0, 0xFF);
        gl::stencilOp(GL_INCR, GL_KEEP, GL_KEEP);
        mPortals[i].draw();

        gl::stencilFunc(GL_NEVER, 0, 0xFF);
        gl::stencilOp(GL_DECR, GL_KEEP, GL_KEEP);
        mPortals[i - 1].draw();
    }
    gl::colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl::depthMask(GL_TRUE);
    gl::stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    gl::stencilFunc(GL_LEQUAL, 1, 0xFF);
    gl::pushViewMatrix();

    mat4 newView = Portal::getNewViewMatrix(mCam.getViewMatrix(), mPortals.back().getModelMatrix(), mPortals.back().getLinkedPortal()->getModelMatrix());
    gl::setViewMatrix(newView);
    drawSceneObjects();
    gl::popViewMatrix();
    gl::disableStencilTest();
}

Camera*
SceneTunnelPortal::getCamera()
{
    return &mCam;
}

void SceneTunnelPortal::handleKeyDown(KeyEvent event)
{
    if (event.getCode() == KeyEvent::KEY_f)
        mCam.toggleFreeze(mGlfwWindowRef);
    if (event.getCode() == KeyEvent::KEY_t)
        mCam.toggleFloating();
}

void SceneTunnelPortal::handleMouseMove(MouseEvent event)
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

void SceneTunnelPortal::processInput()
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
