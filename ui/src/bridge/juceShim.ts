type Completion = (result: unknown) => void;

class PromiseHandler {
  private lastPromiseId = 0;
  private promises = new Map<number, { resolve: Completion }>();

  constructor() {
    if (typeof window === "undefined" || window.__JUCE__ == null) return;

    window.__JUCE__.backend.addEventListener("__juce__complete", (payload: unknown) => {
      const { promiseId, result } = payload as { promiseId: number; result: unknown };
      const entry = this.promises.get(promiseId);
      if (entry) {
        entry.resolve(result);
        this.promises.delete(promiseId);
      }
    });
  }

  createPromise(): [number, Promise<unknown>] {
    const promiseId = this.lastPromiseId++;
    let resolve!: Completion;
    const result = new Promise<unknown>((res) => {
      resolve = res;
    });
    this.promises.set(promiseId, { resolve });
    return [promiseId, result];
  }
}

const promiseHandler = new PromiseHandler();

export function getNativeFunction(name: string): (...args: unknown[]) => Promise<unknown> {
  return (...args: unknown[]) => {
    const [promiseId, result] = promiseHandler.createPromise();
    window.__JUCE__!.backend.emitEvent("__juce__invoke", {
      name,
      params: args,
      resultId: promiseId,
    });
    return result;
  };
}

export function isJuceBackendAvailable(): boolean {
  return typeof window !== "undefined" && window.__JUCE__ != null;
}
