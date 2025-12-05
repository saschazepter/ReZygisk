import { loadPage, setLanguage, reloadPage } from '../../../pageLoader.js'

export async function loadOnce() {

}

export async function loadOnceView() {

}

export async function onceViewAfterUpdate() {

}

export async function load() {
  const sp_lang_close = document.getElementById('sp_lang_close')

  sp_lang_close.addEventListener('click', async function langCloseButtonListener() {
    sp_lang_close.removeEventListener('click', langCloseButtonListener)
    loadPage('settings')
  })
}