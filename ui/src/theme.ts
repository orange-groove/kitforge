import { extendTheme, type ThemeConfig } from "@chakra-ui/react";

const config: ThemeConfig = {
  initialColorMode: "dark",
  useSystemColorMode: false,
};

export const theme = extendTheme({
  config,
  styles: {
    global: {
      "html, body, #root": {
        height: "100%",
        margin: 0,
        padding: 0,
        background: "#121212",
        color: "#e8e8e8",
        overflow: "hidden",
        userSelect: "none",
        WebkitUserSelect: "none",
      },
      // Keep text editable controls usable (caret + typing) despite the
      // app-wide selection lock above.
      "input, textarea, [contenteditable='true']": {
        userSelect: "text",
        WebkitUserSelect: "text",
      },
      ":root": {
        "--audioui-unit": "42px",
        "--audioui-primary-color": "#5b8def",
      },
    },
  },
  colors: {
    kit: {
      bg: "#121212",
      panel: "#1a1a1a",
      border: "#2e2e2e",
      accent: "#5b8def",
      accentMuted: "#3d5a80",
      textMuted: "#9aa0a6",
    },
  },
  fonts: {
    heading: "system-ui, -apple-system, sans-serif",
    body: "system-ui, -apple-system, sans-serif",
  },
});
