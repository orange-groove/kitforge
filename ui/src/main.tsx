import React from "react";
import ReactDOM from "react-dom/client";
import { ChakraProvider } from "@chakra-ui/react";
import { setTheme } from "@cutoff/audio-ui-react";
import "@cutoff/audio-ui-react/style.css";
import App from "./App";
import { theme } from "./theme";

document.documentElement.classList.add("dark");
setTheme({ color: "#5b8def", roundness: 0.35 });

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <ChakraProvider theme={theme}>
      <App />
    </ChakraProvider>
  </React.StrictMode>,
);
