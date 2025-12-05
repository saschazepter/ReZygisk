import { loadPage, setLanguage, reloadPage } from '../../../pageLoader.js'

const availableLanguages = [
  'en_US', // @PerformanC (The PerformanC Organization)
  'vi_VN', // @RainyXeon (Renia in RainyLand)
]

async function _getLanguageData(lang_name) {
  return fetch(`lang/${lang_name}.json`)
    .then((response) => response.json())
    .then((data) => {
      return data
    })
    .catch(() => false)
}

export async function loadOnce() {

}

export async function loadOnceView() {

}

export async function onceViewAfterUpdate() {

}

export async function load() {
  const lang_list = document.getElementById('lang_list')
  // INFO: Language list must be empty before load
  lang_list.innerHTML = ''
  for (let i = 0; i < availableLanguages.length; i++) {
    const langCode = availableLanguages[i]
    const langData = await _getLanguageData(langCode)

    lang_list.innerHTML += `
      <div lang-data="${langCode}" class="dim card card_animation" style="padding: 25px 15px; cursor: pointer;">
        <div lang-data="${langCode}" class="dimc" style="font-size: 1.1em;">${langData.langName}</div>
      </div>
      `
  }

  const sp_lang_close = document.getElementById('sp_lang_close')

  sp_lang_close.addEventListener('click', async function langCloseButtonListener() {
    sp_lang_close.removeEventListener('click', langCloseButtonListener)
    loadPage('settings')
  })

  document.addEventListener('click', async function langButtonListener(event) {
    const getLangLocate = event.target.getAttribute('lang-data')
    if (!getLangLocate || typeof getLangLocate !== 'string') return

    document.removeEventListener('click', langButtonListener)

    setLanguage(getLangLocate)
    loadPage('settings')
    reloadPage()
  }, false)
}