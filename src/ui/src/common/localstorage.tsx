import * as React from 'react';

export const LIVE_VIEW_DATA_DRAWER_OPENED_KEY = 'px-live-data-drawer-opened';
export const LIVE_VIEW_EDITOR_OPENED_KEY = 'px-live-editor-opened';
export const LIVE_VIEW_PIXIE_SCRIPT_KEY = 'px-live-pixie-script';
export const LIVE_VIEW_SCRIPT_TITLE_KEY = 'px-live-script-title';
export const LIVE_VIEW_SCRIPT_ID_KEY = 'px-live-script-id';
export const LIVE_VIEW_VIS_SPEC_KEY = 'px-live-vis';
export const LIVE_VIEW_EDITOR_SPLITS_KEY = 'px-live-editor-splits';
export const LIVE_VIEW_DATA_DRAWER_SPLITS_KEY = 'px-live-data-drawer-splits';

type LocalStorageKey =
  typeof LIVE_VIEW_DATA_DRAWER_OPENED_KEY |
  typeof LIVE_VIEW_DATA_DRAWER_OPENED_KEY |
  typeof LIVE_VIEW_EDITOR_OPENED_KEY |
  typeof LIVE_VIEW_PIXIE_SCRIPT_KEY |
  typeof LIVE_VIEW_SCRIPT_ID_KEY |
  typeof LIVE_VIEW_SCRIPT_TITLE_KEY |
  typeof LIVE_VIEW_VIS_SPEC_KEY |
  typeof LIVE_VIEW_EDITOR_SPLITS_KEY |
  typeof LIVE_VIEW_DATA_DRAWER_SPLITS_KEY;

export function getLiveViewVisSpec(): string {
  return localStorage.getItem(LIVE_VIEW_VIS_SPEC_KEY) || '';
}

export function setLiveViewVisSpec(spec: string) {
  localStorage.setItem(LIVE_VIEW_VIS_SPEC_KEY, spec);
}

export function useLocalStorage<T>(key: LocalStorageKey, initialValue?: T):
  [T, React.Dispatch<React.SetStateAction<T>>] {

  const [state, setState] = React.useState<T>(() => {
    try {
      const stored = localStorage.getItem(key);
      if (stored) {
        return JSON.parse(stored);
      }
    } catch (e) {
      //
    }
    return initialValue;
  });

  // Update the state in localstorage on changes.
  React.useEffect(() => {
    localStorage.setItem(key, JSON.stringify(state));
  }, [state]);

  return [state, setState];
}
