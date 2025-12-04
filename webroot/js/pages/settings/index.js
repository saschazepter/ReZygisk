import { loadPage } from '../pageLoader.js'
import utils from '../utils.js'
import { fullScreen } from '../../kernelsu.js'

function _writeState(ConfigState) {
  return localStorage.setItem('/system/webui_config', JSON.stringify(ConfigState))
}

export async function loadOnce() {

}

export async function loadOnceView() {

}

export async function onceViewAfterUpdate() {

}

export async function load() {
  let ConfigState = {
    disableFullscreen: false,
    enableSystemFont: false
  }

  if (!globalThis.loadedWebUIConfigState) {
    let webui_config = localStorage.getItem('/system/webui_config')

    if (!webui_config) {
      localStorage.setItem('/system/webui_config', JSON.stringify(ConfigState))
    } else {
      ConfigState = JSON.parse(webui_config)
    }
  }

  if (globalThis.loadedModuleConfigState) {
    ConfigState.disableFullscreen = tw_webui_fullscreen_switch.checked
    ConfigState.enableSystemFont = tw_webui_font_switch.checked
  }

  globalThis.loadedWebUIConfigState = true

  const lang_page_toggle = document.getElementById('lang_page_toggle')

  utils.addListener(lang_page_toggle, 'click', async () => {
    loadPage('mini_settings_language')
  })

  const theme_page_toggle = document.getElementById('theme_page_toggle')

  utils.addListener(theme_page_toggle, 'click', async () => {
    loadPage('mini_settings_theme')
  })

  const tw_webui_fullscreen_switch = document.getElementById('tw_webui_fullscreen_switch')
  if (ConfigState.disableFullscreen) tw_webui_fullscreen_switch.checked = true

  utils.addListener(tw_webui_fullscreen_switch, 'click', () => {
    /* INFO: This is swapped, as it meant to disable the fullscreen */
    ConfigState.disableFullscreen = !ConfigState.disableFullscreen
    _writeState(ConfigState)

    fullScreen(!ConfigState.disableFullscreen)
  })

  const tw_webui_font_switch = document.getElementById('tw_webui_font_switch')
  if (ConfigState.enableSystemFont) tw_webui_font_switch.checked = true

  utils.addListener(tw_webui_font_switch, 'click', () => {
    /* INFO: This is swapped, as it meant to enable the fullscreen */
    ConfigState.enableSystemFont = !ConfigState.enableSystemFont
  
    if (ConfigState.enableSystemFont) {
      const headTag = document.getElementsByTagName('head')[0]
      const styleTag = document.createElement('style')

      styleTag.id = 'font-tag'
      headTag.appendChild(styleTag)
      styleTag.innerHTML = `
        :root {
          --font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif
        }`
    } else {
      document.getElementById('font-tag').remove()
    }

    _writeState(ConfigState)
  })
}