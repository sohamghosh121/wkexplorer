/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebInspectorClient.h"

#if ENABLE(INSPECTOR)

#include "DrawingArea.h"
#include "WebInspector.h"
#include "WebPage.h"
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>
#include <wtf/CurrentTime.h>

#if PLATFORM(IOS)
#include <WebCore/InspectorOverlay.h>
#endif

using namespace WebCore;

namespace WebKit {

class RepaintIndicatorLayerClient final : public GraphicsLayerClient {
public:
    RepaintIndicatorLayerClient(WebInspectorClient& inspectorClient)
        : m_inspectorClient(inspectorClient)
    {
    }
    virtual ~RepaintIndicatorLayerClient() { }
private:
    virtual void notifyAnimationEnded(const GraphicsLayer* layer, const String&) override
    {
        m_inspectorClient.animationEndedForLayer(layer);
    }
    
    WebInspectorClient& m_inspectorClient;
};

WebInspectorClient::WebInspectorClient(WebPage* page)
    : m_page(page)
    , m_highlightOverlay(nullptr)
{
}

WebInspectorClient::~WebInspectorClient()
{
    for (auto layer : m_paintRectLayers) {
        layer->removeFromParent();
        delete layer;
    }

    if (m_paintRectOverlay)
        m_page->uninstallPageOverlay(m_paintRectOverlay.get());
}

void WebInspectorClient::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

WebCore::InspectorFrontendChannel* WebInspectorClient::openInspectorFrontend(InspectorController* controller)
{
    WebPage* inspectorPage = controller->isUnderTest() ? m_page->inspector()->createInspectorPageForTest() : m_page->inspector()->createInspectorPage();
    ASSERT_UNUSED(inspectorPage, inspectorPage);
    return this;
}

void WebInspectorClient::closeInspectorFrontend()
{
    if (m_page->inspector())
        m_page->inspector()->didClose();
}

void WebInspectorClient::bringFrontendToFront()
{
    m_page->inspector()->bringToFront();
}

void WebInspectorClient::didResizeMainFrame(Frame*)
{
    if (m_page->inspector())
        m_page->inspector()->updateDockingAvailability();
}

void WebInspectorClient::highlight()
{
#if !PLATFORM(IOS)
    if (!m_highlightOverlay) {
        RefPtr<PageOverlay> highlightOverlay = PageOverlay::create(this);
        m_highlightOverlay = highlightOverlay.get();
        m_page->installPageOverlay(highlightOverlay.release(), PageOverlay::FadeMode::Fade);
        m_highlightOverlay->setNeedsDisplay();
    } else {
        m_highlightOverlay->stopFadeOutAnimation();
        m_highlightOverlay->setNeedsDisplay();
    }
#else
    Highlight highlight;
    m_page->corePage()->inspectorController().getHighlight(&highlight, InspectorOverlay::CoordinateSystem::Document);
    m_page->showInspectorHighlight(highlight);
#endif
}

void WebInspectorClient::hideHighlight()
{
#if !PLATFORM(IOS)
    if (m_highlightOverlay)
        m_page->uninstallPageOverlay(m_highlightOverlay, PageOverlay::FadeMode::Fade);
#else
    m_page->hideInspectorHighlight();
#endif
}

void WebInspectorClient::showPaintRect(const FloatRect& rect)
{
    if (!m_paintRectOverlay) {
        m_paintRectOverlay = PageOverlay::create(this, PageOverlay::OverlayType::Document);
        m_page->installPageOverlay(m_paintRectOverlay, PageOverlay::FadeMode::DoNotFade);
    }

    if (!m_paintIndicatorLayerClient)
        m_paintIndicatorLayerClient = std::make_unique<RepaintIndicatorLayerClient>(*this);

    std::unique_ptr<GraphicsLayer> paintLayer = GraphicsLayer::create(m_page->drawingArea()->graphicsLayerFactory(), *m_paintIndicatorLayerClient);
    
    paintLayer->setAnchorPoint(FloatPoint3D());
    paintLayer->setPosition(rect.location());
    paintLayer->setSize(rect.size());
    paintLayer->setBackgroundColor(Color(1.0f, 0.0f, 0.0f, 0.2f));

    KeyframeValueList fadeKeyframes(AnimatedPropertyOpacity);
    OwnPtr<AnimationValue> intialValue = FloatAnimationValue::create(0, 1);
    fadeKeyframes.insert(intialValue.release());

    OwnPtr<AnimationValue> finalValue = FloatAnimationValue::create(0.25, 0);
    fadeKeyframes.insert(finalValue.release());
    
    RefPtr<Animation> opacityAnimation = Animation::create();
    opacityAnimation->setDuration(0.25);

    paintLayer->addAnimation(fadeKeyframes, FloatSize(), opacityAnimation.get(), ASCIILiteral("opacity"), 0);
    
    m_paintRectLayers.add(paintLayer.get());

    GraphicsLayer* overlayRootLayer = m_paintRectOverlay->layer();
    overlayRootLayer->addChild(paintLayer.release());
}

void WebInspectorClient::animationEndedForLayer(const GraphicsLayer* layer)
{
    const_cast<GraphicsLayer*>(layer)->removeFromParent();
    m_paintRectLayers.remove(const_cast<GraphicsLayer*>(layer));
    delete layer;
}

#if PLATFORM(IOS)
void WebInspectorClient::showInspectorIndication()
{
    m_page->showInspectorIndication();
}

void WebInspectorClient::hideInspectorIndication()
{
    m_page->hideInspectorIndication();
}

void WebInspectorClient::didSetSearchingForNode(bool enabled)
{
    if (enabled)
        m_page->enableInspectorNodeSearch();
    else
        m_page->disableInspectorNodeSearch();
}
#endif

bool WebInspectorClient::sendMessageToFrontend(const String& message)
{
    WebInspector* inspector = m_page->inspector();
    if (!inspector)
        return false;

#if ENABLE(INSPECTOR_SERVER)
    if (inspector->hasRemoteFrontendConnected()) {
        inspector->sendMessageToRemoteFrontend(message);
        return true;
    }
#endif

    WebPage* inspectorPage = inspector->inspectorPage();
    if (inspectorPage)
        return doDispatchMessageOnFrontendPage(inspectorPage->corePage(), message);

    return false;
}

bool WebInspectorClient::supportsFrameInstrumentation()
{
#if USE(COORDINATED_GRAPHICS)
    return true;
#endif
    return false;
}

void WebInspectorClient::pageOverlayDestroyed(PageOverlay*)
{
}

void WebInspectorClient::willMoveToWebPage(PageOverlay*, WebPage* webPage)
{
    if (webPage)
        return;

    // The page overlay is moving away from the web page, reset it.
    ASSERT(m_highlightOverlay);
    m_highlightOverlay = 0;
}

void WebInspectorClient::didMoveToWebPage(PageOverlay*, WebPage*)
{
}

void WebInspectorClient::drawRect(PageOverlay*, WebCore::GraphicsContext& context, const WebCore::IntRect& /*dirtyRect*/)
{
    m_page->corePage()->inspectorController().drawHighlight(context);
}

bool WebInspectorClient::mouseEvent(PageOverlay*, const WebMouseEvent&)
{
    return false;
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
