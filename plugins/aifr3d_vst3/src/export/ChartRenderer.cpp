#include "ChartRenderer.h"

#include <array>

namespace aifr3d::plugin {

namespace {

double optOr(const std::optional<double>& v, double fallback = -60.0) {
  return v.has_value() ? *v : fallback;
}

}  // namespace

juce::Image ChartRenderer::renderSpectralBands(const AnalysisSnapshot& snapshot, int width, int height) {
  juce::Image img(juce::Image::ARGB, width, height, true);
  juce::Graphics g(img);
  auto area = img.getBounds();
  drawPanelBackground(g, area);

  const std::array<std::pair<const char*, std::optional<double>>, 7> bands{{
      {"Sub", snapshot.analysis.spectral.sub},
      {"Low", snapshot.analysis.spectral.low},
      {"LowMid", snapshot.analysis.spectral.lowmid},
      {"Mid", snapshot.analysis.spectral.mid},
      {"HiMid", snapshot.analysis.spectral.highmid},
      {"High", snapshot.analysis.spectral.high},
      {"Air", snapshot.analysis.spectral.air},
  }};

  const int margin = 16;
  auto plot = area.reduced(margin);
  const int barW = juce::jmax(8, plot.getWidth() / static_cast<int>(bands.size() * 2));
  const int step = juce::jmax(barW + 8, plot.getWidth() / static_cast<int>(bands.size()));

  g.setColour(juce::Colours::white.withAlpha(0.85f));
  g.setFont(14.0f);
  g.drawText("Spectral Bands", plot.removeFromTop(24), juce::Justification::left);

  auto barsArea = plot;
  const float minDb = -70.0f;
  const float maxDb = 10.0f;
  int x = barsArea.getX();
  for (const auto& [name, value] : bands) {
    const float db = juce::jlimit(minDb, maxDb, static_cast<float>(optOr(value, minDb)));
    const float norm = (db - minDb) / (maxDb - minDb);
    const int h = static_cast<int>(norm * static_cast<float>(barsArea.getHeight() - 30));
    const juce::Rectangle<int> bar(x, barsArea.getBottom() - h - 20, barW, h);
    g.setGradientFill(juce::ColourGradient(accentColor(), static_cast<float>(bar.getX()), static_cast<float>(bar.getBottom()),
                                           juce::Colours::orange.withAlpha(0.8f), static_cast<float>(bar.getRight()),
                                           static_cast<float>(bar.getY()), false));
    g.fillRoundedRectangle(bar.toFloat(), 3.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText(name, x - 6, barsArea.getBottom() - 18, barW + 16, 16, juce::Justification::centred);
    x += step;
  }

  return img;
}

juce::Image ChartRenderer::renderLoudnessTruePeak(const AnalysisSnapshot& snapshot, int width, int height) {
  juce::Image img(juce::Image::ARGB, width, height, true);
  juce::Graphics g(img);
  auto area = img.getBounds();
  drawPanelBackground(g, area);

  g.setColour(juce::Colours::white.withAlpha(0.85f));
  g.setFont(14.0f);
  auto title = area.reduced(16).removeFromTop(24);
  g.drawText("Loudness + True Peak", title, juce::Justification::left);

  auto meterArea = area.reduced(20).withTrimmedTop(30);
  auto left = meterArea.removeFromLeft(meterArea.getWidth() / 2).reduced(8);
  auto right = meterArea.reduced(8);

  const auto drawBar = [&](juce::Rectangle<int> r, const juce::String& label, double db, double minDb, double maxDb) {
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillRoundedRectangle(r.toFloat(), 4.0f);
    const float norm = static_cast<float>((juce::jlimit(minDb, maxDb, db) - minDb) / (maxDb - minDb));
    auto fill = r.withHeight(static_cast<int>(norm * static_cast<float>(r.getHeight()))).withY(
        r.getBottom() - static_cast<int>(norm * static_cast<float>(r.getHeight())));
    g.setColour(accentColor());
    g.fillRoundedRectangle(fill.toFloat(), 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawText(label, r.removeFromTop(18), juce::Justification::centred);
    g.drawText(juce::String(db, 2) + " dB", r.removeFromBottom(18), juce::Justification::centred);
  };

  drawBar(left, "Integrated LUFS", optOr(snapshot.analysis.loudness.integrated_lufs, -70.0), -70.0, 0.0);
  drawBar(right, "True Peak", optOr(snapshot.analysis.true_peak.true_peak_dbfs, -12.0), -12.0, 1.0);

  return img;
}

juce::Image ChartRenderer::renderStereoCorrelation(const AnalysisSnapshot& snapshot, int width, int height) {
  juce::Image img(juce::Image::ARGB, width, height, true);
  juce::Graphics g(img);
  auto area = img.getBounds();
  drawPanelBackground(g, area);

  g.setColour(juce::Colours::white.withAlpha(0.85f));
  g.setFont(14.0f);
  g.drawText("Stereo Correlation", area.reduced(16).removeFromTop(24), juce::Justification::left);

  auto meter = area.reduced(24).withTrimmedTop(36);
  g.setColour(juce::Colours::white.withAlpha(0.2f));
  g.fillRoundedRectangle(meter.toFloat(), 6.0f);

  const float corr = static_cast<float>(juce::jlimit(-1.0, 1.0, optOr(snapshot.analysis.stereo.correlation, 0.0)));
  const int centerX = meter.getCentreX();
  const int span = meter.getWidth() / 2;
  const int markerX = centerX + static_cast<int>(corr * static_cast<float>(span));

  g.setColour(juce::Colours::white.withAlpha(0.3f));
  g.drawVerticalLine(centerX, static_cast<float>(meter.getY()), static_cast<float>(meter.getBottom()));
  g.setColour(accentColor());
  g.fillEllipse(static_cast<float>(markerX - 6), static_cast<float>(meter.getCentreY() - 6), 12.0f, 12.0f);

  g.setColour(juce::Colours::white.withAlpha(0.9f));
  g.drawText("-1", meter.getX(), meter.getBottom() - 18, 24, 16, juce::Justification::left);
  g.drawText("0", centerX - 6, meter.getBottom() - 18, 12, 16, juce::Justification::centred);
  g.drawText("+1", meter.getRight() - 24, meter.getBottom() - 18, 24, 16, juce::Justification::right);

  return img;
}

void ChartRenderer::drawPanelBackground(juce::Graphics& g, juce::Rectangle<int> area) {
  g.fillAll(juce::Colour::fromRGB(16, 19, 24));
  g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(36, 40, 52),
                                         static_cast<float>(area.getX()),
                                         static_cast<float>(area.getY()),
                                         juce::Colour::fromRGB(20, 23, 30),
                                         static_cast<float>(area.getRight()),
                                         static_cast<float>(area.getBottom()),
                                         true));
  g.fillRoundedRectangle(area.toFloat().reduced(2.0f), 10.0f);
}

juce::Colour ChartRenderer::accentColor() { return juce::Colour::fromRGB(82, 163, 255); }

}  // namespace aifr3d::plugin
