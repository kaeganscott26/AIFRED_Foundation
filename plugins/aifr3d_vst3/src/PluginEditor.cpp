#include "PluginEditor.h"

#include "export/ReportExporter.h"

namespace aifr3d::plugin {

namespace {

juce::String fmtOptDb(const std::optional<double>& v, int decimals = 2) {
  return v.has_value() ? (juce::String(*v, decimals) + " dB") : "n/a";
}

juce::String fmtOpt(const std::optional<double>& v, int decimals = 2) {
  return v.has_value() ? juce::String(*v, decimals) : "n/a";
}

juce::Colour backgroundTop() { return juce::Colour::fromRGB(20, 22, 28); }
juce::Colour backgroundBottom() { return juce::Colour::fromRGB(10, 11, 15); }
juce::Colour cardColor() { return juce::Colour::fromRGB(31, 35, 44); }
juce::Colour accent() { return juce::Colour::fromRGB(82, 163, 255); }

class MetricBarsComponent final : public juce::Component {
 public:
  void setSnapshot(std::shared_ptr<const AnalysisSnapshot> snapshot) {
    snapshot_ = std::move(snapshot);
    repaint();
  }

  void paint(juce::Graphics& g) override {
    g.fillAll(cardColor().withAlpha(0.9f));
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(14.0f);
    g.drawText("Key Metric Bars", getLocalBounds().removeFromTop(22), juce::Justification::left);

    if (snapshot_ == nullptr || !snapshot_->valid) {
      g.setColour(juce::Colours::white.withAlpha(0.65f));
      g.drawText("No analysis loaded.", getLocalBounds().reduced(10), juce::Justification::centred);
      return;
    }

    struct Row {
      juce::String label;
      double value;
      double min;
      double max;
    };

    const auto& a = snapshot_->analysis;
    std::array<Row, 5> rows{{
        {"Integrated LUFS", a.loudness.integrated_lufs.value_or(-70.0), -70.0, 0.0},
        {"True Peak", a.true_peak.true_peak_dbfs.value_or(-12.0), -12.0, 1.0},
        {"Stereo Corr", a.stereo.correlation.value_or(0.0), -1.0, 1.0},
        {"Width", a.stereo.width_proxy.value_or(0.0), 0.0, 1.0},
        {"Overall Score", snapshot_->score.has_value() ? snapshot_->score->overall_0_100 : 0.0, 0.0, 100.0},
    }};

    auto area = getLocalBounds().reduced(10).withTrimmedTop(24);
    const int rowH = juce::jmax(28, area.getHeight() / static_cast<int>(rows.size()));
    for (const auto& row : rows) {
      auto r = area.removeFromTop(rowH).reduced(2);
      g.setColour(juce::Colours::white.withAlpha(0.18f));
      g.fillRoundedRectangle(r.toFloat(), 3.0f);
      g.setColour(juce::Colours::white.withAlpha(0.9f));
      g.drawText(row.label, r.removeFromLeft(160), juce::Justification::centredLeft);
      const float norm = static_cast<float>((juce::jlimit(row.min, row.max, row.value) - row.min) / (row.max - row.min));
      auto barBg = r.reduced(6, 5);
      g.setColour(juce::Colours::white.withAlpha(0.15f));
      g.fillRoundedRectangle(barBg.toFloat(), 3.0f);
      auto fill = barBg.withWidth(static_cast<int>(norm * static_cast<float>(barBg.getWidth())));
      g.setColour(accent());
      g.fillRoundedRectangle(fill.toFloat(), 3.0f);
      g.setColour(juce::Colours::white.withAlpha(0.9f));
      g.drawText(juce::String(row.value, 2), r.removeFromRight(80), juce::Justification::centredRight);
    }
  }

 private:
  std::shared_ptr<const AnalysisSnapshot> snapshot_;
};

class DualSourceMeterComponent final : public juce::Component {
 public:
  void setSnapshot(std::shared_ptr<const AnalysisSnapshot> snapshot) {
    snapshot_ = std::move(snapshot);
    repaint();
  }
  void setMode(int modeIndex) {
    modeIndex_ = modeIndex;
    repaint();
  }

  void paint(juce::Graphics& g) override {
    g.fillAll(cardColor().withAlpha(0.9f));
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(14.0f);
    g.drawText("Reference-Aware Metering™ (2-source)", getLocalBounds().removeFromTop(24), juce::Justification::left);

    auto area = getLocalBounds().reduced(18).withTrimmedTop(24);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.fillRoundedRectangle(area.toFloat(), 6.0f);

    const auto drawValueMarker = [&](double value, double min, double max, juce::Colour c, const juce::String& label, int yOff) {
      const float norm = static_cast<float>((juce::jlimit(min, max, value) - min) / (max - min));
      const int x = area.getX() + static_cast<int>(norm * static_cast<float>(area.getWidth()));
      g.setColour(c);
      g.drawLine(static_cast<float>(x), static_cast<float>(area.getY() + 8 + yOff),
                 static_cast<float>(x), static_cast<float>(area.getBottom() - 8), 2.0f);
      g.drawText(label + ": " + juce::String(value, 2), x - 80, area.getY() + yOff, 160, 16, juce::Justification::centred);
    };

    if (snapshot_ == nullptr || !snapshot_->valid) {
      g.setColour(juce::Colours::white.withAlpha(0.7f));
      g.drawText("Run analysis to populate meter overlays.", area, juce::Justification::centred);
      return;
    }

    const double track = snapshot_->analysis.loudness.integrated_lufs.value_or(-70.0);
    const double proPool = snapshot_->benchmarkCompare.has_value() &&
                                   snapshot_->benchmarkCompare->loudness.integrated_lufs.mean.has_value()
                               ? *snapshot_->benchmarkCompare->loudness.integrated_lufs.mean
                               : -14.0;
    const double userRef = snapshot_->referenceCompare.has_value() &&
                                   snapshot_->referenceCompare->reference_average.loudness.integrated_lufs.has_value()
                               ? *snapshot_->referenceCompare->reference_average.loudness.integrated_lufs
                               : -14.0;

    if (modeIndex_ == 0) {
      drawValueMarker(track, -30.0, 0.0, accent(), "True Meter", 2);
      drawValueMarker(proPool, -30.0, 0.0, juce::Colours::orange, "Pro Pool", 18);
    } else if (modeIndex_ == 1) {
      drawValueMarker(track, -30.0, 0.0, accent(), "True Meter", 2);
      drawValueMarker(userRef, -30.0, 0.0, juce::Colours::lightgreen, "User Ref", 18);
    } else {
      drawValueMarker(userRef, -30.0, 0.0, juce::Colours::lightgreen, "User Ref", 2);
      drawValueMarker(proPool, -30.0, 0.0, juce::Colours::orange, "Pro Pool", 18);
    }
  }

 private:
  std::shared_ptr<const AnalysisSnapshot> snapshot_;
  int modeIndex_{0};
};

}  // namespace

Aifr3dAudioProcessorEditor::Aifr3dAudioProcessorEditor(Aifr3dAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor), processor_(processor) {
  setSize(1220, 780);

  titleLabel_.setText("AIFR3D — Reference-Aware Metering", juce::dontSendNotification);
  titleLabel_.setFont(juce::Font(23.0f, juce::Font::bold));
  titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(titleLabel_);

  subtitleLabel_.setText("Phase 6 VST3 Wrapper", juce::dontSendNotification);
  subtitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.65f));
  addAndMakeVisible(subtitleLabel_);

  statusLabel_.setText("Idle", juce::dontSendNotification);
  statusLabel_.setJustificationType(juce::Justification::centredRight);
  statusLabel_.setColour(juce::Label::textColourId, accent());
  addAndMakeVisible(statusLabel_);

  tabs_.setColour(juce::TabbedComponent::backgroundColourId, cardColor());
  tabs_.addTab("AIFRED", juce::Colours::transparentBlack, &tabAifred_, false);
  tabs_.addTab("Analysis", juce::Colours::transparentBlack, &tabAnalysis_, false);
  tabs_.addTab("Compare", juce::Colours::transparentBlack, &tabCompare_, false);
  tabs_.addTab("Report", juce::Colours::transparentBlack, &tabReport_, false);
  tabs_.addTab("Settings", juce::Colours::transparentBlack, &tabSettings_, false);
  addAndMakeVisible(tabs_);

  analyzeBufferButton_.addListener(this);
  analyzeFileButton_.addListener(this);
  cancelAnalysisButton_.addListener(this);
  exportButton_.addListener(this);
  pickBenchmarkButton_.addListener(this);
  pickReferenceButton_.addListener(this);

  headerCard_.setColour(juce::Label::backgroundColourId, cardColor().withAlpha(0.92f));
  headerCard_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
  headerCard_.setJustificationType(juce::Justification::centredLeft);
  headerCard_.setText("No analysis yet", juce::dontSendNotification);

  tabAifred_.addAndMakeVisible(analyzeBufferButton_);
  tabAifred_.addAndMakeVisible(analyzeFileButton_);
  tabAifred_.addAndMakeVisible(cancelAnalysisButton_);
  tabAifred_.addAndMakeVisible(headerCard_);

  metricBarsLabel_.setJustificationType(juce::Justification::topLeft);
  metricBarsLabel_.setColour(juce::Label::backgroundColourId, cardColor().withAlpha(0.9f));
  metricBarsLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
  metricBarsLabel_.setText("Run analysis to view metrics.", juce::dontSendNotification);
  tabAnalysis_.addAndMakeVisible(metricBarsLabel_);
  metricBarsComponent_ = std::make_unique<MetricBarsComponent>();
  tabAnalysis_.addAndMakeVisible(*metricBarsComponent_);

  meterModeLabel_.setText("Meter Mode (2-source overlay):", juce::dontSendNotification);
  meterModeLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
  tabCompare_.addAndMakeVisible(meterModeLabel_);

  meterModeCombo_.addItem("True Meter vs Pro Pool", 1);
  meterModeCombo_.addItem("True Meter vs User Ref", 2);
  meterModeCombo_.addItem("User Ref vs Pro Pool", 3);
  tabCompare_.addAndMakeVisible(meterModeCombo_);
  meterModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      processor_.state(), "meterMode", meterModeCombo_);

  compareLabel_.setJustificationType(juce::Justification::topLeft);
  compareLabel_.setColour(juce::Label::backgroundColourId, cardColor().withAlpha(0.9f));
  compareLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
  compareLabel_.setText("No compare data yet.", juce::dontSendNotification);
  tabCompare_.addAndMakeVisible(compareLabel_);
  dualMeterComponent_ = std::make_unique<DualSourceMeterComponent>();
  tabCompare_.addAndMakeVisible(*dualMeterComponent_);

  issuesLabel_.setJustificationType(juce::Justification::topLeft);
  issuesLabel_.setColour(juce::Label::backgroundColourId, cardColor().withAlpha(0.9f));
  issuesLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
  issuesLabel_.setText("Top issues will appear here.", juce::dontSendNotification);
  tabCompare_.addAndMakeVisible(issuesLabel_);

  reportStatusLabel_.setColour(juce::Label::backgroundColourId, cardColor().withAlpha(0.9f));
  reportStatusLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
  reportStatusLabel_.setText("Ready to export", juce::dontSendNotification);
  reportStatusLabel_.setJustificationType(juce::Justification::centredLeft);
  tabReport_.addAndMakeVisible(reportStatusLabel_);
  tabReport_.addAndMakeVisible(exportButton_);

  sessionHistory_.setMultiLine(true);
  sessionHistory_.setReadOnly(true);
  sessionHistory_.setColour(juce::TextEditor::backgroundColourId, cardColor().withAlpha(0.9f));
  sessionHistory_.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.9f));
  sessionHistory_.setText("Session history:\n");
  tabReport_.addAndMakeVisible(sessionHistory_);

  benchmarkPathLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
  benchmarkPathLabel_.setText("Benchmark: (none)", juce::dontSendNotification);
  referencePathLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
  referencePathLabel_.setText("User Ref: (none)", juce::dontSendNotification);
  tabSettings_.addAndMakeVisible(benchmarkPathLabel_);
  tabSettings_.addAndMakeVisible(referencePathLabel_);
  tabSettings_.addAndMakeVisible(pickBenchmarkButton_);
  tabSettings_.addAndMakeVisible(pickReferenceButton_);

  startTimerHz(8);
}

Aifr3dAudioProcessorEditor::~Aifr3dAudioProcessorEditor() {
  analyzeBufferButton_.removeListener(this);
  analyzeFileButton_.removeListener(this);
  cancelAnalysisButton_.removeListener(this);
  exportButton_.removeListener(this);
  pickBenchmarkButton_.removeListener(this);
  pickReferenceButton_.removeListener(this);
}

void Aifr3dAudioProcessorEditor::paint(juce::Graphics& g) {
  g.setGradientFill(juce::ColourGradient(backgroundTop(), 0.0f, 0.0f, backgroundBottom(), 0.0f,
                                         static_cast<float>(getHeight()), false));
  g.fillAll();

  g.setColour(juce::Colours::black.withAlpha(0.35f));
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 14.0f);
}

void Aifr3dAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(18);
  auto top = area.removeFromTop(48);
  titleLabel_.setBounds(top.removeFromLeft(700));
  subtitleLabel_.setBounds(top.removeFromLeft(230));
  statusLabel_.setBounds(top);

  tabs_.setBounds(area);

  auto aifred = tabAifred_.getLocalBounds().reduced(14);
  headerCard_.setBounds(aifred.removeFromTop(84));
  auto btnRow = aifred.removeFromTop(40);
  analyzeBufferButton_.setBounds(btnRow.removeFromLeft(220));
  btnRow.removeFromLeft(8);
  analyzeFileButton_.setBounds(btnRow.removeFromLeft(200));
  btnRow.removeFromLeft(8);
  cancelAnalysisButton_.setBounds(btnRow.removeFromLeft(180));

  metricBarsLabel_.setBounds(tabAnalysis_.getLocalBounds().reduced(14));
  metricBarsComponent_->setBounds(metricBarsLabel_.getBounds().withTrimmedBottom(metricBarsLabel_.getHeight() / 2));
  metricBarsLabel_.setBounds(metricBarsLabel_.getBounds().withTrimmedTop(metricBarsComponent_->getBottom() - metricBarsLabel_.getY() + 8));

  auto cmp = tabCompare_.getLocalBounds().reduced(14);
  auto modeRow = cmp.removeFromTop(28);
  meterModeLabel_.setBounds(modeRow.removeFromLeft(250));
  meterModeCombo_.setBounds(modeRow.removeFromLeft(280));
  cmp.removeFromTop(8);
  auto topCmp = cmp.removeFromTop(cmp.getHeight() / 2);
  dualMeterComponent_->setBounds(topCmp);
  cmp.removeFromTop(8);
  compareLabel_.setBounds(cmp.removeFromTop(cmp.getHeight() / 2));
  cmp.removeFromTop(8);
  issuesLabel_.setBounds(cmp);

  auto report = tabReport_.getLocalBounds().reduced(14);
  auto rTop = report.removeFromTop(34);
  reportStatusLabel_.setBounds(rTop.removeFromLeft(680));
  exportButton_.setBounds(rTop.removeFromLeft(200));
  report.removeFromTop(10);
  sessionHistory_.setBounds(report);

  auto settings = tabSettings_.getLocalBounds().reduced(14);
  auto bRow = settings.removeFromTop(34);
  benchmarkPathLabel_.setBounds(bRow.removeFromLeft(760));
  pickBenchmarkButton_.setBounds(bRow.removeFromLeft(180));
  settings.removeFromTop(8);
  auto rRow = settings.removeFromTop(34);
  referencePathLabel_.setBounds(rRow.removeFromLeft(760));
  pickReferenceButton_.setBounds(rRow.removeFromLeft(180));
}

void Aifr3dAudioProcessorEditor::timerCallback() { refreshFromSnapshot(); }

void Aifr3dAudioProcessorEditor::buttonClicked(juce::Button* button) {
  if (button == &analyzeBufferButton_) {
    processor_.triggerAnalysisFromCapturedBuffer();
    statusLabel_.setText("Running captured-buffer analysis...", juce::dontSendNotification);
    return;
  }
  if (button == &analyzeFileButton_) {
    juce::FileChooser chooser("Select WAV file", {}, "*.wav");
    if (chooser.browseForFileToOpen()) {
      const auto file = chooser.getResult();
      processor_.triggerAnalysisFromFile(file);
      statusLabel_.setText("Running offline WAV analysis...", juce::dontSendNotification);
    }
    return;
  }
  if (button == &cancelAnalysisButton_) {
    processor_.cancelAnalysisJobs();
    statusLabel_.setText("Analysis cancel requested.", juce::dontSendNotification);
    return;
  }
  if (button == &exportButton_) {
    if (lastSnapshot_ == nullptr || !lastSnapshot_->valid) {
      reportStatusLabel_.setText("No valid analysis to export.", juce::dontSendNotification);
      return;
    }
    auto result = ReportExporter::exportSnapshot(*lastSnapshot_, juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
    if (!result.ok) {
      reportStatusLabel_.setText("Export failed: " + result.error, juce::dontSendNotification);
      return;
    }
    processor_.setLastSessionId(result.sessionId);
    processor_.setLastExportPath(result.sessionDir.getFullPathName());
    reportStatusLabel_.setText("Exported: " + result.sessionDir.getFullPathName(), juce::dontSendNotification);
    appendSessionHistory(result.sessionId + "  ->  " + result.sessionDir.getFullPathName());
    return;
  }
  if (button == &pickBenchmarkButton_) {
    juce::FileChooser chooser("Select benchmark profile JSON", {}, "*.json");
    if (chooser.browseForFileToOpen()) {
      const auto file = chooser.getResult();
      processor_.setBenchmarkProfilePath(file.getFullPathName());
      benchmarkPathLabel_.setText("Benchmark: " + file.getFullPathName(), juce::dontSendNotification);
    }
    return;
  }
  if (button == &pickReferenceButton_) {
    juce::FileChooser chooser("Select user reference WAV", {}, "*.wav");
    if (chooser.browseForFileToOpen()) {
      const auto file = chooser.getResult();
      processor_.setReferenceWavPath(file.getFullPathName());
      referencePathLabel_.setText("User Ref: " + file.getFullPathName(), juce::dontSendNotification);
    }
  }
}

void Aifr3dAudioProcessorEditor::refreshFromSnapshot() {
  auto snapshot = processor_.latestSnapshot();
  if (snapshot == nullptr) {
    return;
  }
  if (snapshot == lastSnapshot_ || snapshot->generation == lastSeenGeneration_) {
    return;
  }

  lastSnapshot_ = snapshot;
  lastSeenGeneration_ = snapshot->generation;

  const auto perf = processor_.perfCounters();
  if (!snapshot->valid) {
    if (snapshot->canceled) {
      statusLabel_.setText("Analysis canceled (gen " + juce::String(static_cast<int>(snapshot->generation)) + ")",
                           juce::dontSendNotification);
    } else if (snapshot->timedOut) {
      statusLabel_.setText("Analysis timed out", juce::dontSendNotification);
    } else {
      statusLabel_.setText("Analysis failed: " + snapshot->errorMessage, juce::dontSendNotification);
    }
    return;
  }

  statusLabel_.setText("Analysis ready | ms=" + juce::String(snapshot->processingMs, 1) +
                           " avg=" + juce::String(perf.avgJobMs, 1) +
                           " dropped=" + juce::String(static_cast<int>(perf.droppedPendingJobs)),
                       juce::dontSendNotification);

  const juce::String scoreText = snapshot->score.has_value()
                                     ? juce::String(snapshot->score->overall_0_100, 1)
                                     : juce::String("n/a");

  headerCard_.setText("Track: " + snapshot->trackName + "\n"
                          "Sample Rate: " + juce::String(snapshot->sampleRateHz, 1) + " Hz\n"
                          "Duration: " + juce::String(snapshot->durationSeconds, 2) + " s\n"
                          "Overall Score: " + scoreText,
                      juce::dontSendNotification);

  metricBarsLabel_.setText(
      "Analysis Metrics\n\n"
      "Integrated LUFS: " + fmtOpt(snapshot->analysis.loudness.integrated_lufs) + "\n"
      "True Peak: " + fmtOptDb(snapshot->analysis.true_peak.true_peak_dbfs) + "\n"
      "Stereo Correlation: " + fmtOpt(snapshot->analysis.stereo.correlation) + "\n"
      "Width Proxy: " + fmtOpt(snapshot->analysis.stereo.width_proxy) + "\n"
      "Sub: " + fmtOptDb(snapshot->analysis.spectral.sub) + "\n"
      "Low: " + fmtOptDb(snapshot->analysis.spectral.low) + "\n"
      "Mid: " + fmtOptDb(snapshot->analysis.spectral.mid) + "\n"
      "Air: " + fmtOptDb(snapshot->analysis.spectral.air),
      juce::dontSendNotification);

  juce::String compareText = "Compare\n\n";
  const int modeIndex = meterModeCombo_.getSelectedItemIndex();
  dualMeterComponent_->setMode(modeIndex);
  if (modeIndex == 0) {
    compareText << "Mode: True Meter vs Pro Pool\n";
    compareText << "True Meter (Track): LUFS " << fmtOpt(snapshot->analysis.loudness.integrated_lufs) << ", TP "
                << fmtOptDb(snapshot->analysis.true_peak.true_peak_dbfs) << "\n";
    if (snapshot->benchmarkCompare.has_value()) {
      compareText << "Pro Pool delta LUFS: " << fmtOpt(snapshot->benchmarkCompare->loudness.integrated_lufs.delta)
                  << "\n";
    } else {
      compareText << "Pro Pool: not loaded\n";
    }
  } else if (modeIndex == 1) {
    compareText << "Mode: True Meter vs User Ref\n";
    compareText << "True Meter (Track): LUFS " << fmtOpt(snapshot->analysis.loudness.integrated_lufs) << "\n";
    if (snapshot->referenceCompare.has_value()) {
      compareText << "User Ref closeness: "
                  << juce::String(snapshot->referenceCompare->distance.overall_closeness_0_100, 1) << "\n";
    } else {
      compareText << "User Ref: not loaded\n";
    }
  } else {
    compareText << "Mode: User Ref vs Pro Pool\n";
    compareText << "User Ref and Pro Pool shown via reference/benchmark summaries.\n";
    if (snapshot->referenceCompare.has_value()) {
      compareText << "User Ref overall closeness: "
                  << juce::String(snapshot->referenceCompare->distance.overall_closeness_0_100, 1) << "\n";
    }
    if (snapshot->benchmarkCompare.has_value()) {
      compareText << "Pro Pool in-range metrics: " << snapshot->benchmarkCompare->summary.in_range_count << "\n";
    }
  }
  compareLabel_.setText(compareText, juce::dontSendNotification);
  metricBarsComponent_->setSnapshot(snapshot);
  dualMeterComponent_->setSnapshot(snapshot);

  juce::String issueText = "Top 5 Issues\n\n";
  if (snapshot->issues.has_value() && !snapshot->issues->top_issues.empty()) {
    for (std::size_t i = 0; i < snapshot->issues->top_issues.size() && i < 5U; ++i) {
      const auto& issue = snapshot->issues->top_issues[i];
      issueText << juce::String(static_cast<int>(i + 1)) << ". " << issue.title << " ("
                << ReportExporter::severityToString(issue.severity) << ")\n"
                << "   Evidence: "
                << (issue.evidence.empty() ? juce::String("n/a") : juce::String(issue.evidence.front().metric_path))
                << "\n"
                << "   Fix: "
                << (issue.fix_steps.empty() ? juce::String("n/a") : juce::String(issue.fix_steps.front().action))
                << "\n\n";
    }
  } else {
    issueText << "No triggered issues.";
  }
  issuesLabel_.setText(issueText, juce::dontSendNotification);
}

void Aifr3dAudioProcessorEditor::appendSessionHistory(const juce::String& line) {
  sessionHistory_.moveCaretToEnd();
  sessionHistory_.insertTextAtCaret(line + "\n");
}

}  // namespace aifr3d::plugin
