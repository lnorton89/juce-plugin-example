#include "LumaScope/WebResources.h"
#include "TestWebResources.h"
#include <iostream>

namespace
{
int failures = 0;
void expect (bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main()
{
    lumascope::WebResources resources (TestWebResourcesData::webfixture_zip,
                                       TestWebResourcesData::webfixture_zipSize);
    const auto html = resources.get ("/");
    const auto js = resources.get ("/app.js?cache=1");
    const auto css = resources.get ("/style.css#theme");
    expect (html && html->mimeType == "text/html", "serves index HTML");
    expect (js && js->mimeType == "text/javascript", "serves JavaScript and strips query");
    expect (css && css->mimeType == "text/css", "serves CSS and strips fragment");
    expect (! resources.get ("/../index.html"), "rejects traversal");
    expect (! resources.get ("C:/index.html"), "rejects absolute paths");
    expect (! resources.get ("/unknown.bin"), "rejects unknown MIME type");
    expect (! resources.get ("/missing.js"), "rejects missing resources");
    return failures == 0 ? 0 : 1;
}
