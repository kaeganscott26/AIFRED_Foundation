

package com.aifred.mobile;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Message;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.speech.RecognitionListener;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.webkit.ConsoleMessage;
import android.webkit.CookieManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.util.ArrayList;
import java.util.Locale;
import java.util.UUID;

public class MainActivity extends Activity {

    private static final int REQ_MIC_PERMISSION = 2001;

    private WebView webView;

    private TextToSpeech tts;
    private boolean ttsReady = false;

    private SpeechRecognizer speechRecognizer;
    private boolean sttReady = false;
    private boolean sttListening = false;

    @SuppressLint({"SetJavaScriptEnabled"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        webView = new WebView(this);
        setContentView(webView);

        configureWebView(webView);

        // Enable WebView debugging (remove if you want)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WebView.setWebContentsDebuggingEnabled(true);
        }

        // JS bridge object name: AIFRED_ANDROID
        webView.addJavascriptInterface(new AifredBridge(), "AIFRED_ANDROID");

        initTTS();
        initSTT();

        webView.loadUrl("file:///android_asset/www/index.html");
    }

    private void configureWebView(WebView wv) {
        WebSettings settings = wv.getSettings();

        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setDatabaseEnabled(true);

        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);

        // Needed for file:///android_asset to call https endpoints
        settings.setAllowFileAccessFromFileURLs(true);
        settings.setAllowUniversalAccessFromFileURLs(true);

        settings.setJavaScriptCanOpenWindowsAutomatically(true);
        settings.setSupportMultipleWindows(true);

        // Let audio play after user gesture (our UI has buttons)
        settings.setMediaPlaybackRequiresUserGesture(true);

        // Performance-ish defaults
        settings.setLoadsImagesAutomatically(true);
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(true);
        settings.setUserAgentString(settings.getUserAgentString() + " AIFREDAndroidWebView/1.0");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        }

        CookieManager cm = CookieManager.getInstance();
        cm.setAcceptCookie(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            cm.setAcceptThirdPartyCookies(wv, true);
        }

        wv.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                Uri uri = request != null ? request.getUrl() : null;
                if (uri == null) return false;

                String scheme = uri.getScheme() == null ? "" : uri.getScheme().toLowerCase();
                if ("http".equals(scheme) || "https".equals(scheme) || "file".equals(scheme)) {
                    return false; // keep in WebView
                }
                openExternal(uri);
                return true;
            }

            @Override
            @SuppressWarnings("deprecation")
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                if (url == null) return false;
                Uri uri = Uri.parse(url);
                String scheme = uri.getScheme() == null ? "" : uri.getScheme().toLowerCase();
                if ("http".equals(scheme) || "https".equals(scheme) || "file".equals(scheme)) {
                    return false;
                }
                openExternal(uri);
                return true;
            }
        });

        wv.setWebChromeClient(new WebChromeClient() {
            @Override
            public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
                // Helpful for debugging in logcat
                return super.onConsoleMessage(consoleMessage);
            }

            @Override
            public boolean onCreateWindow(WebView view, boolean isDialog, boolean isUserGesture, Message resultMsg) {
                WebView popup = new WebView(MainActivity.this);
                configureWebView(popup);

                popup.setWebViewClient(new WebViewClient() {
                    @Override
                    public boolean shouldOverrideUrlLoading(WebView popupView, WebResourceRequest request) {
                        Uri uri = request != null ? request.getUrl() : null;
                        if (uri != null) openExternal(uri);
                        return true;
                    }

                    @Override
                    @SuppressWarnings("deprecation")
                    public boolean shouldOverrideUrlLoading(WebView popupView, String url) {
                        if (url != null) openExternal(Uri.parse(url));
                        return true;
                    }
                });

                WebView.WebViewTransport transport = (WebView.WebViewTransport) resultMsg.obj;
                transport.setWebView(popup);
                resultMsg.sendToTarget();
                return true;
            }
        });
    }

    private void openExternal(Uri uri) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, uri));
        } catch (ActivityNotFoundException ignored) {}
    }

    // ---------------------------
    // TTS
    // ---------------------------
    private void initTTS() {
        tts = new TextToSpeech(this, status -> {
            ttsReady = (status == TextToSpeech.SUCCESS);
            if (ttsReady) {
                tts.setLanguage(Locale.US);
                tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                    @Override public void onStart(String utteranceId) { jsTtsEvent("start"); }
                    @Override public void onDone(String utteranceId)  { jsTtsEvent("done"); }
                    @Override public void onError(String utteranceId) { jsTtsEvent("error"); }
                });
            }
            jsCall("window.__AIFRED_onTtsReady && window.__AIFRED_onTtsReady(" + (ttsReady ? "true" : "false") + ");");
        });
    }

    private void jsTtsEvent(String evt) {
        jsCall("window.__AIFRED_onTtsEvent && window.__AIFRED_onTtsEvent('" + escapeJs(evt) + "');");
    }

    // ---------------------------
    // STT
    // ---------------------------
    private void initSTT() {
        try {
            speechRecognizer = SpeechRecognizer.createSpeechRecognizer(this);
            sttReady = true;

            speechRecognizer.setRecognitionListener(new RecognitionListener() {
                @Override public void onReadyForSpeech(Bundle params) { jsSttEvent("ready"); }
                @Override public void onBeginningOfSpeech() { jsSttEvent("begin"); }
                @Override public void onRmsChanged(float rmsdB) {}
                @Override public void onBufferReceived(byte[] buffer) {}
                @Override public void onEndOfSpeech() { jsSttEvent("end"); }

                @Override public void onError(int error) {
                    sttListening = false;
                    jsCall("window.__AIFRED_onSttError && window.__AIFRED_onSttError(" + error + ");");
                }

                @Override public void onResults(Bundle results) {
                    sttListening = false;
                    ArrayList<String> matches = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
                    String text = (matches != null && !matches.isEmpty()) ? matches.get(0) : "";
                    jsCall("window.__AIFRED_onSttResult && window.__AIFRED_onSttResult('" + escapeJs(text) + "');");
                }

                @Override public void onPartialResults(Bundle partialResults) {}
                @Override public void onEvent(int eventType, Bundle params) {}
            });

        } catch (Exception e) {
            sttReady = false;
        }

        jsCall("window.__AIFRED_onSttReady && window.__AIFRED_onSttReady(" + (sttReady ? "true" : "false") + ");");
    }

    private void jsSttEvent(String evt) {
        jsCall("window.__AIFRED_onSttEvent && window.__AIFRED_onSttEvent('" + escapeJs(evt) + "');");
    }

    private boolean hasMicPermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true;
        return checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
    }

    private void requestMicPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, REQ_MIC_PERMISSION);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQ_MIC_PERMISSION) {
            boolean ok = grantResults != null && grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
            jsCall("window.__AIFRED_onMicPermission && window.__AIFRED_onMicPermission(" + (ok ? "true" : "false") + ");");
        }
    }

    // ---------------------------
    // JS BRIDGE (callable from JS as AIFRED_ANDROID.*)
    // ---------------------------
    private class AifredBridge {

        @JavascriptInterface
        public boolean isTtsReady() { return ttsReady; }

        @JavascriptInterface
        public boolean isSttReady() { return sttReady; }

        @JavascriptInterface
        public void ensureMicPermission() {
            if (!hasMicPermission()) {
                requestMicPermission();
            } else {
                jsCall("window.__AIFRED_onMicPermission && window.__AIFRED_onMicPermission(true);");
            }
        }

        @JavascriptInterface
        public void ttsSpeak(String text, float rate, float pitch) {
            if (!ttsReady || tts == null) return;
            if (text == null) text = "";

            float safeRate = clamp(rate, 0.1f, 2.0f);
            float safePitch = clamp(pitch, 0.5f, 2.0f);

            tts.setSpeechRate(safeRate);
            tts.setPitch(safePitch);

            String utteranceId = UUID.randomUUID().toString();
            tts.speak(text, TextToSpeech.QUEUE_FLUSH, null, utteranceId);
        }

        @JavascriptInterface
        public void ttsStop() {
            if (tts != null) tts.stop();
        }

        @JavascriptInterface
        public void sttStart(String langTag) {
            if (!sttReady || speechRecognizer == null) return;

            if (!hasMicPermission()) {
                requestMicPermission();
                return;
            }

            if (sttListening) return;
            sttListening = true;

            Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
            intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
            intent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, false);

            if (langTag != null && !langTag.trim().isEmpty()) {
                intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, langTag.trim());
            } else {
                intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault());
            }

            speechRecognizer.startListening(intent);
        }

        @JavascriptInterface
        public void sttStop() {
            if (speechRecognizer != null) {
                try { speechRecognizer.stopListening(); } catch (Exception ignored) {}
            }
            sttListening = false;
        }
    }

    // ---------------------------
    // Helpers
    // ---------------------------
    private void jsCall(String js) {
        if (webView == null) return;
        runOnUiThread(() -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                webView.evaluateJavascript(js, null);
            } else {
                webView.loadUrl("javascript:" + js);
            }
        });
    }

    private static String escapeJs(String s) {
        if (s == null) return "";
        return s.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n").replace("\r", "\\r");
    }

    private static float clamp(float v, float min, float max) {
        return Math.max(min, Math.min(max, v));
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
            return;
        }
        super.onBackPressed();
    }

    @Override
    protected void onDestroy() {
        try {
            if (speechRecognizer != null) {
                speechRecognizer.destroy();
                speechRecognizer = null;
            }
        } catch (Exception ignored) {}

        try {
            if (tts != null) {
                tts.shutdown();
                tts = null;
            }
        } catch (Exception ignored) {}

        try {
            if (webView != null) {
                webView.destroy();
                webView = null;
            }
        } catch (Exception ignored) {}

        super.onDestroy();
    }
}