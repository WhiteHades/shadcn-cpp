"""Regression check for clang-doc output rendered by Docusaurus."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from api_markdown import normalize


class ApiMarkdownTest(unittest.TestCase):
    def test_cpp_signatures_enums_and_qt_metadata(self):
        output = normalize("""# class Button
*Defined at include/shadcn/widgets.hpp#56*
## Members
public static QMetaObject staticMetaObject
## Functions
### Button
*public void Button(const QString & text, QWidget * parent)*
### result
*public std::expected<void, ValueError> result()*
### tr
*public static QString tr(const char * s)*
## Enums
| enum class Variant |
--
| Default |
| Outline |
*Defined at include/shadcn/core.hpp#14*
""")
        self.assertIn("```cpp\npublic Button(", output)
        self.assertIn("```cpp\npublic std::expected<void, ValueError>", output)
        self.assertIn("### `Variant`", output)
        self.assertIn("- `Outline`", output)
        self.assertIn("widgets.hpp#L56", output)
        self.assertNotIn("QMetaObject", output)
        self.assertNotIn("QString tr", output)
        self.assertNotIn("void Button", output)
        self.assertNotIn("```cpp\n```cpp", output)


if __name__ == "__main__":
    unittest.main()
