/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
/* Your own sounds' folder, from inside the plugin:
 * - "Folder" opens it in Explorer, Finder or the file manager;
 * - "Import..." takes in sounds: the files themselves, or a zip of them (the zip's folders do not matter, files that
 *   are not the plugin's sounds are left out). A sound never replaces one already there: the very same file is
 *   skipped, another under the same name gets a name of its own ("NAME (2)"). */
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>

namespace pfio
{
enum { OPEN = 2, IMPORT = 3 };   /* the two buttons' results in a dialog (1 and 0: the dialog's own) */

struct Result
{
    int added = 0, same = 0, other = 0, failed = 0;
};

/* the folder in Explorer / Finder / the file manager */
inline void open(const juce::File &folder)
{
    folder.createDirectory();
    folder.startAsProcess();
}

/* one sound into the folder (its bytes) */
inline void put(const juce::File &folder, const juce::String &fileName, const juce::MemoryBlock &data, Result &r)
{
    auto f = folder.getChildFile(juce::File::createLegalFileName(fileName));
    if (f.existsAsFile()) {
        juce::MemoryBlock old;
        if (f.loadFileAsData(old) && old == data) {
            r.same++;
            return;
        }
        f = folder.getNonexistentChildFile(f.getFileNameWithoutExtension(), f.getFileExtension(), true);
    }
    if (f.replaceWithData(data.getData(), data.getSize()))
        r.added++;
    else
        r.failed++;
}

/* files and zips into the folder; ext: the sounds' extension (".p67"); zipToBank: a zip's sounds into a folder of
 * their own, named after the zip (for plugins that show such folders as banks) */
inline Result importFiles(const juce::Array<juce::File> &files, const juce::File &folder, const juce::String &ext, bool zipToBank = false)
{
    constexpr int MAXBYTES = 4 << 20;   /* a sound is a few kB: anything bigger is not one */
    Result r;
    folder.createDirectory();
    auto wanted = [&ext](const juce::String &name) { return name.endsWithIgnoreCase(ext) && !name.startsWith("._"); };
    for (auto &f : files) {
        if (f.hasFileExtension("zip")) {
            juce::ZipFile zip(f);
            auto dest = folder;
            if (zipToBank) {
                dest = folder.getChildFile(juce::File::createLegalFileName(f.getFileNameWithoutExtension()));
                dest.createDirectory();
            }
            for (int i = 0; i < zip.getNumEntries(); i++) {
                auto *e = zip.getEntry(i);
                /* only its name: where it was inside the zip does not count (nor can it point outside the folder) */
                auto name = e->filename.replaceCharacter('\\', '/').fromLastOccurrenceOf("/", false, false);
                if (name.isEmpty())
                    continue;                              /* a folder */
                if (!wanted(name) || e->isSymbolicLink || e->filename.contains("__MACOSX") || e->uncompressedSize > MAXBYTES) {
                    r.other++;
                    continue;
                }
                std::unique_ptr<juce::InputStream> in(zip.createStreamForEntry(i));
                juce::MemoryBlock mb;
                if (in == nullptr || in->readIntoMemoryBlock(mb, MAXBYTES) == 0) {
                    r.failed++;
                    continue;
                }
                put(dest, name, mb, r);
            }
            if (zipToBank && dest.findChildFiles(juce::File::findFiles, false).isEmpty())
                dest.deleteFile();                     /* (no sounds in it: no empty bank) */
        } else if (wanted(f.getFileName()) && f.getSize() <= MAXBYTES) {
            juce::MemoryBlock mb;
            if (f.loadFileAsData(mb))
                put(folder, f.getFileName(), mb, r);
            else
                r.failed++;
        } else
            r.other++;
    }
    return r;
}

/* what an import did, in a few words; word: what the plugin calls a sound ("preset", "patch", "program") */
inline juce::String summary(const Result &r, const juce::String &word)
{
    auto n = [&word](int k) { return juce::String(k) + " " + word + (k == 1 ? "" : (word.endsWith("ch") ? "es" : "s")); };
    juce::String s = r.added > 0 ? n(r.added) + " imported." : "Nothing new imported.";
    if (r.same > 0)
        s << "\n" << n(r.same) << " already there (left as they are).";
    if (r.other > 0)
        s << "\n" << r.other << (r.other == 1 ? " file was" : " files were") << " not a " << word << " (left out).";
    if (r.failed > 0)
        s << "\n" << r.failed << (r.failed == 1 ? " file" : " files") << " could not be read or written.";
    return s;
}

/* the plugin's window (a dialog of its own could open behind the host's plugin window, unseen) */
inline juce::Component &window(juce::Component &c)
{
    if (dynamic_cast<juce::AudioProcessorEditor *>(&c) != nullptr)
        return c;
    if (auto *e = c.findParentComponentOfClass<juce::AudioProcessorEditor>())
        return *e;
    return c;
}

/* a message inside the plugin's window, with OK */
inline void tell(juce::Component &owner, const juce::String &title, const juce::String &text)
{
    auto &h = window(owner);
    auto *w = new juce::AlertWindow(title, text, juce::MessageBoxIconType::NoIcon);
    w->setLookAndFeel(&owner.getLookAndFeel());
    w->addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
    h.addAndMakeVisible(w);
    w->setCentrePosition(h.getLocalBounds().getCentre());
    w->toFront(true);
    w->enterModalState(true, nullptr, true);
}

/* "Import...": pick sound files or zips (it starts in Downloads), they go into the folder, then reload() (the plugin
 * reads its folder again) and a message says what came in */
inline void choose(juce::Component &owner, const juce::File &folder, const juce::String &ext, const juce::String &word,
                   std::function<void()> reload, bool zipToBank = false)
{
    auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    auto start = home.getChildFile("Downloads").isDirectory() ? home.getChildFile("Downloads") : home;
    auto *fc = new juce::FileChooser("Import " + word + "s (" + ext + " files, or a .zip of them)", start, "*" + ext + ";*.zip",
                                     true, false, &window(owner));
    juce::Component::SafePointer<juce::Component> sp(&owner);
    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::canSelectMultipleItems,
                    [fc, sp, folder, ext, word, reload, zipToBank](const juce::FileChooser &c) {
                        auto files = c.getResults();
                        if (!files.isEmpty()) {
                            auto r = importFiles(files, folder, ext, zipToBank);
                            if (sp != nullptr) {
                                if (r.added > 0 && reload)
                                    reload();
                                tell(*sp, "IMPORT", summary(r, word));
                            }
                        }
                        juce::MessageManager::callAsync([fc] { delete fc; });
                    });
}

/* a dialog's result: OPEN or IMPORT handled (true), else false */
inline bool handle(int result, juce::Component &owner, const juce::File &folder, const juce::String &ext, const juce::String &word,
                   std::function<void()> reload, bool zipToBank = false)
{
    if (result == OPEN)
        open(folder);
    else if (result == IMPORT)
        juce::MessageManager::callAsync([sp = juce::Component::SafePointer<juce::Component>(&owner), folder, ext, word, reload, zipToBank] {
            if (sp != nullptr)   /* (after the dialog is gone) */
                choose(*sp, folder, ext, word, reload, zipToBank);
        });
    else
        return false;
    return true;
}

/* the two buttons, for a dialog (after its own) */
inline void addButtons(juce::AlertWindow &w)
{
    w.addButton("Folder", OPEN);
    w.addButton("Import...", IMPORT);
}
} // namespace pfio
