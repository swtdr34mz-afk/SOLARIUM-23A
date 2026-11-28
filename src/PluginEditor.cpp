/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
#include "PluginEditor.h"
#include "PanelDialog.h"
#include "PatchFolderIO.h"
#include "WindowSize.h"

using namespace s23;
using namespace juce;

/* the header's controls: the program (click: the list), its arrows, SAVE, the readout */
struct SolarisEditor::Header : Component {
    SolarisEditor &ed;
    explicit Header(SolarisEditor &e) : ed(e) {}
    Rectangle<float> logo() const { return {22.0f, 10.0f, 330.0f, 44.0f}; }
    Rectangle<float> prog() const { return {560.0f, 15.0f, 300.0f, 32.0f}; }
    Rectangle<float> prev() const { return {522.0f, 18.0f, 26.0f, 26.0f}; }
    Rectangle<float> next() const { return {872.0f, 18.0f, 26.0f, 26.0f}; }
    Rectangle<float> save() const { return {912.0f, 18.0f, 26.0f, 26.0f}; }
    Rectangle<float> read() const { return {1000.0f, 15.0f, 544.0f, 32.0f}; }
    void paint(Graphics &g) override
    {
        drawLogo(g, logo());
        drawLegend(g, juce::String::fromUTF8("drone  ·  ambient\nmicrotonal"), {360.0f, 16.0f, 140.0f, 32.0f}, 10.5f);
        drawLcd(g, prog());
        g.setColour(lcdText());
        g.setFont(font(Face::mono, 17.0f));
        int p = ed.proc.getCurrentProgram();
        g.drawText(String(p + 1).paddedLeft('0', 2) + "  " + ed.proc.getProgramName(p), prog().reduced(10.0f, 0.0f),
                   Justification::centredLeft, true);
        for (int d : {-1, 1}) {
            auto a = d < 0 ? prev() : next();
            drawButton(g, a, false, false, false);
            Path t;
            auto c = a.getCentre().translated(0.0f, -20.0f);
            t.addTriangle(c.x + 5.0f * (float)d, c.y, c.x - 4.0f * (float)d, c.y - 5.0f, c.x - 4.0f * (float)d, c.y + 5.0f);
            g.setColour(ink());
            g.fillPath(t);
        }
        drawButton(g, save(), false, false, true);
        drawLegend(g, "save", save().translated(0.0f, -20.0f), 10.0f);
        drawLcd(g, read());
        if (ed.inHand) {
            g.setFont(font(Face::mono, 14.0f));
            g.setColour(lcdText().withAlpha(0.75f));
            g.drawText(ed.inHand->getName(40), read().reduced(12.0f, 0.0f), Justification::centredLeft, true);
            g.setColour(lcdText());
            g.drawText(ed.inHand->getCurrentValueAsText(), read().reduced(12.0f, 0.0f), Justification::centredRight, true);
        }
    }
    void mouseDown(const MouseEvent &e) override
    {
        if (e.mods.isPopupMenu())        /* (a right click changes nothing) */
            return;
        int n = ed.proc.programCount(), p = ed.proc.getCurrentProgram();
        if (prog().contains(e.position))
            ed.programMenu();
        else if (prev().expanded(4.0f).contains(e.position))
            ed.proc.setCurrentProgram((p + n - 1) % n);
        else if (next().expanded(4.0f).contains(e.position))
            ed.proc.setCurrentProgram((p + 1) % n);
        else if (save().expanded(4.0f).contains(e.position))
            ed.saveProgram();
        repaint();
    }
};

SolarisEditor::SolarisEditor(SolarisProcessor &p) : AudioProcessorEditor(&p), proc(p)
{
    acquireResources();
    setFinish(proc.uiFinish.load());
    setLookAndFeel(&look);
    Builder b{proc.apvts, page, this};
    b.keys = &proc.keyboardState;
    b.meters.light = [this] { return proc.light.load(); };
    b.meters.lfo = [this](int l) { return proc.lfo[l].load(); };
    b.meters.voice = [this](int v) { return proc.voice[v].load(); };
    b.meters.step = [this] { return proc.seqStep.load(); };

    /* row 1: DRONE 1, DRONE 2, the effector, DRONE 4, DRONE 5 */
    b.classic(1, {16, 64, 300, 290});
    b.classic(2, {324, 64, 300, 290});
    b.effector({632, 64, 296, 290});
    b.classic(4, {936, 64, 300, 290});
    b.classic(5, {1244, 64, 300, 290});
    /* row 2: DRONE 3, VCO A, the mixer and the filter, VCO B, DRONE 6 */
    b.chaos(3, {16, 362, 300, 290});
    b.vco(0, {324, 362, 300, 290});
    b.mixer({632, 362, 296, 140});
    b.filter({632, 510, 296, 142});
    b.vco(1, {936, 362, 300, 290});
    b.chaos(6, {1244, 362, 300, 290});
    /* row 3: the modulation */
    b.lfo(0, {16, 660, 190, 150});
    b.joystick({214, 660, 270, 150});
    b.sequencer({492, 660, 636, 150});
    b.light({1136, 660, 210, 150});
    b.lfo(1, {1354, 660, 190, 150});
    /* row 4: the touch plates, the drone keys, glide and tuning */
    b.plates({16, 818, 1040, 140});
    b.droneKeys({1064, 818, 260, 140});
    b.global({1332, 818, 212, 140});
    page.extra = [](Graphics &g) {
        drawTag(g, {1330.0f, 966.0f, 200.0f, 26.0f});
        drawLegend(g, juce::String::fromUTF8("MICROTONAL DRONE MACHINE  ·  No. 23  ·  swtdr34mz"), {24.0f, 968.0f, 360.0f, 22.0f}, 10.0f);
    };
    for (float sx : {10.0f, 780.0f, 1550.0f})   /* the panel's screws */
        for (float sy : {10.0f, 990.0f})
            page.screws.push_back({sx, sy});
    page.setSize(W, H);
    addAndMakeVisible(page);
    header = std::make_unique<Header>(*this);
    header->setBounds(0, 0, W, 60);
    page.addAndMakeVisible(*header);
    makeShade();                         /* (the section zoom's veil, hidden until a section is zoomed) */
    page.onSection = [this](int i) { zoomTo(i); };
    for (auto *c : page.getChildren())
        if (dynamic_cast<Lamp *>(c) || dynamic_cast<Plates *>(c))
            lamps.push_back(c);

    /* the size it was left at, read before the window is made resizable; a new instance (still at the default
     * size): the size last used anywhere */
    float s = proc.uiScale.load();
    if (std::abs(s - 0.8f) < 1e-4f)
        s = juce::jlimit(0.5f, 1.5f, (float)winsize::last("SOLARIUM-23A", 0.8));
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((double)W / (double)H);
    setResizeLimits(W / 2, H / 2, (int)(W * 1.5f), (int)(H * 1.5f));
    sized = true;
    setSize((int)std::lround(W * s), (int)std::lround(H * s));
    startTimerHz(30);
}

SolarisEditor::~SolarisEditor()
{
    stopTimer();
    page.removeChildComponent(header.get());   /* (the header is the editor's own; the page deletes the rest) */
    page.deleteAllChildren();
    setLookAndFeel(nullptr);
    look.setDefaultSansSerifTypeface(nullptr);
    releaseResources();                  /* (the last editor: the fonts and pictures go now, not at unload) */
}

/* the zoom's veil: the panel around the shown section darkened (a click there, or on the X, shows the whole panel
   again); the section itself takes the clicks as ever */
struct SolarisEditor::Shade : Component {
    SolarisEditor &ed;
    Rectangle<float> hole;               /* (the section, in the editor's coordinates) */
    explicit Shade(SolarisEditor &e) : ed(e) { setInterceptsMouseClicks(true, false); }
    Rectangle<float> closeBox() const
    {
        float d = jlimit(26.0f, 40.0f, hole.getWidth() * 0.05f);
        return {hole.getRight() - d * 0.55f, hole.getY() - d * 0.45f, d, d};
    }
    bool hitTest(int x, int y) override
    {
        Point<float> p((float)x, (float)y);
        return closeBox().contains(p) || !hole.contains(p);
    }
    void paint(Graphics &g) override
    {
        Path veil;
        veil.addRectangle(getLocalBounds().toFloat());
        veil.addRoundedRectangle(hole.expanded(3.0f), 6.0f);
        veil.setUsingNonZeroWinding(false);
        g.setColour(Colours::black.withAlpha(0.72f));
        g.fillPath(veil);
        auto c = closeBox();             /* the X: a round bone-coloured button */
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillEllipse(c.translated(0.0f, 2.0f));
        g.setColour(Colour(0xffd8d2c4));
        g.fillEllipse(c);
        g.setColour(Colour(0xff151515));
        auto x = c.reduced(c.getWidth() * 0.32f);
        g.drawLine({x.getTopLeft(), x.getBottomRight()}, 2.4f);
        g.drawLine({x.getTopRight(), x.getBottomLeft()}, 2.4f);
    }
    void mouseDown(const MouseEvent &) override { ed.zoomTo(-1); }
    void mouseMove(const MouseEvent &e) override
    {
        setMouseCursor(closeBox().contains(e.position) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }
};

void SolarisEditor::makeShade()
{
    shade = std::make_unique<Shade>(*this);
    addChildComponent(*shade);
}

AffineTransform SolarisEditor::pageTransform(int section) const
{
    const float s = (float)getWidth() / (float)W;
    if (section < 0 || section >= (int)page.sections.size())
        return AffineTransform::scale(s);
    /* the section (and a margin) as large as the window allows, in its middle; at most 4 times the panel's size */
    auto r = page.sections[(size_t)section].r.expanded(14.0f);
    float z = jmin((float)getWidth() / r.getWidth(), (float)getHeight() / r.getHeight(), s * 4.0f);
    float tx = ((float)getWidth() - r.getWidth() * z) * 0.5f - r.getX() * z;
    float ty = ((float)getHeight() - r.getHeight() * z) * 0.5f - r.getY() * z;
    return AffineTransform::scale(z).translated(tx, ty);
}

void SolarisEditor::zoomTo(int section)
{
    if (section == zoomed)
        section = -1;                    /* (its tab again: back) */
    zoomed = section;
    animFrom = page.getTransform();
    animTo = pageTransform(section);
    animStart = Time::getMillisecondCounterHiRes() - (isShowing() ? 0.0 : 1000.0);   /* (not on screen: no animation) */
    getProperties().set("liveResize", true);   /* (while it moves: everything only stretched) */
    page.setZoom({});
    if (section >= 0) {
        shade->setVisible(true);
        shade->toFront(false);
    }
    anim.startTimerHz(60);
    animate();
}

void SolarisEditor::animate()
{
    double t = jlimit(0.0, 1.0, (Time::getMillisecondCounterHiRes() - animStart) / 220.0);
    float e = (float)(1.0 - std::pow(1.0 - t, 3.0));   /* (eases out) */
    auto lerp = [e](float a, float b) { return a + (b - a) * e; };
    AffineTransform m(lerp(animFrom.mat00, animTo.mat00), 0.0f, lerp(animFrom.mat02, animTo.mat02),
                      0.0f, lerp(animFrom.mat11, animTo.mat11), lerp(animFrom.mat12, animTo.mat12));
    page.setTransform(m);
    auto *sh = static_cast<Shade *>(shade.get());
    if (zoomed >= 0) {
        sh->hole = page.sections[(size_t)zoomed].r.transformedBy(m);
        sh->repaint();
    }
    if (t >= 1.0) {
        anim.stopTimer();
        getProperties().set("liveResize", false);
        if (zoomed >= 0)
            page.setZoom(page.sections[(size_t)zoomed].r);
        else
            sh->setVisible(false);
        page.repaint();
    }
}

void SolarisEditor::paint(Graphics &g)
{
    if (frozen.isValid()) {              /* (being resized: the panel's picture, stretched) */
        g.setImageResamplingQuality(Graphics::lowResamplingQuality);
        g.drawImage(frozen, getLocalBounds().toFloat());
    } else
        g.fillAll(Colours::black);
}

void SolarisEditor::resized()
{
    float s = (float)getWidth() / (float)W;
    if (sized && zoomed < 0 && !frozen.isValid() && page.isVisible() && page.getWidth() > 0) {
        /* the window is being dragged larger or smaller: the panel as one picture, taken once at the size it had,
           only stretched while it moves (drawing ~200 controls afresh at every step made it judder); drawn sharp
           again once it has come to rest */
        float before = page.getTransform().getScaleFactor();
        float dpi = 1.0f;
        if (auto *d = Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))
            dpi = (float)d->scale;
        frozen = page.createComponentSnapshot(page.getLocalBounds(), true, jlimit(0.5f, 4.0f, before * dpi));
        page.setVisible(false);
    }
    page.setTransform(pageTransform(zoomed));
    if (shade) {
        shade->setBounds(getLocalBounds());
        if (zoomed >= 0)
            static_cast<Shade *>(shade.get())->hole = page.sections[(size_t)zoomed].r.transformedBy(page.getTransform());
    }
    if (sized) {
        getProperties().set("liveResize", true);
        resizeTicks = 0;
        repaint();
    }
    if (!sized)
        return;
    proc.uiScale = s;
    winsize::remember("SOLARIUM-23A", (double)s);
}

void SolarisEditor::settle()
{
    getProperties().set("liveResize", false);
    frozen = {};
    page.setVisible(true);
    page.repaint();
}

void SolarisEditor::timerCallback()
{
    if ((bool)getProperties()["liveResize"] && !anim.isTimerRunning() && ++resizeTicks > 8)   /* ~0.25 s at rest: sharp again */
        settle();
    for (auto *c : lamps) {              /* only what has moved */
        if (auto *l = dynamic_cast<Lamp *>(c)) {
            if (l->changed())
                l->repaint();
        } else if (auto *p = dynamic_cast<Plates *>(c)) {
            if (p->changed())
                p->repaint();
        } else
            c->repaint();
    }
    /* the header: only when its program or the value in hand changes */
    int pr = proc.getCurrentProgram();
    String key = String(pr) + proc.getProgramName(pr) + (inHand ? inHand->getName(40) + inHand->getCurrentValueAsText() : String());
    if (key != headerKey) {
        headerKey = key;
        header->repaint();
    }
}

void SolarisEditor::touched(RangedAudioParameter &p)
{
    inHand = &p;
    header->repaint();
}

void SolarisEditor::programMenu()
{
    PopupMenu m;
    for (int i = 0; i < sol::numPresets(); i++)
        m.addItem(i + 1, String(i + 1).paddedLeft('0', 2) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    if (!proc.userPresets.empty()) {
        m.addSeparator();
        m.addSectionHeader("YOUR OWN");
        for (int i = sol::numPresets(); i < proc.programCount(); i++)
            m.addItem(i + 1, String(i + 1).paddedLeft('0', 2) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    }
    m.addSeparator();
    m.addItem(-2, "Open the presets folder");
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(header.get()), [this](int r) {
        if (r > 0)
            proc.setCurrentProgram(r - 1);
        else if (r == -2) {
            SolarisProcessor::presetFolder().createDirectory();
            SolarisProcessor::presetFolder().startAsProcess();
        }
        header->repaint();
    });
}

void SolarisEditor::finishMenu()
{
    PopupMenu m;
    m.addSectionHeader("THE PANEL'S FINISH");
    for (int f = 0; f < NFINISH; f++)
        m.addItem(f + 1, finishName(f), true, f == finish());
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(header.get()), [this](int r) {
        if (r > 0) {
            setFinish(r - 1);
            proc.uiFinish = r - 1;
            page.repaint();
        }
    });
}

void SolarisEditor::saveProgram()
{
    auto *w = new AlertWindow("SAVE", "The sound as it is now, under a name of its own (the same name again replaces it):",
                              MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.getProgramName(proc.getCurrentProgram()));
    w->addButton("SAVE", 1, KeyPress(KeyPress::returnKey));
    w->addButton("CANCEL", 0, KeyPress(KeyPress::escapeKey));
    pfio::addButtons(*w);   /* Folder, Import... */
    pdlg::showInside(*this, w);
    w->enterModalState(true, ModalCallbackFunction::create([this, w](int r) {
                           if (pfio::handle(r, *this, SolarisProcessor::presetFolder(), ".s23", "preset", [this] { proc.rescanUser(); header->repaint(); }))
                               return;
                           if (r == 1)
                               pdlg::checked(proc.writePreset(w->getTextEditorContents("name")), *this);
                           header->repaint();
                       }),
                       true);
}

juce::AudioProcessorEditor *SolarisProcessor::createEditor() { return new SolarisEditor(*this); }
