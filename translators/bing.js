var lastText = '';
var active = window.location.href !== 'about:blank';

function findOutput() {
  var sels = [
    '#tta_output_ta',
    'textarea[aria-label="Translation"]',
    '[data-testid="translator-output"]'
  ];
  for (var i = 0; i < sels.length; ++i) {
    var el = document.querySelector(sels[i]);
    if (el) {
      return (el.value !== undefined ? el.value : el.innerText || '').trim();
    }
  }
  return '';
}

function checkFinished() {
  if (!active) return;
  var text = findOutput();
  if (text === lastText || text === '' || text === '...') return;
  console.log('translated text', text, 'old', lastText, 'size', text.length, lastText.length);
  lastText = text;
  active = false;
  proxy.setTranslated(text);
}

function translate(text, from, to) {
  console.log('start translate', text, from, to);
  if (text.trim().length === 0) {
    proxy.setTranslated('');
    return;
  }
  active = true;

  var supported = ['auto','ar','bg','zh-Hans','zh-Hant','hr','cs','da','nl','en','et','fi','fr','de','el','he','hi','hu','id','it','ja','ko','lv','lt','no','pl','pt','ro','ru','sk','sl','es','sv','th','tr','uk','vi'];
  if (supported.indexOf(from) === -1) {
    proxy.setFailed('Source language not supported');
    return;
  }
  if (supported.indexOf(to) === -1) {
    proxy.setFailed('Target language not supported');
    return;
  }

  if (window.location.href.indexOf('www.bing.com/translator') !== -1
      && window.location.href.indexOf('&to=' + to) !== -1) {
    var input = document.querySelector('#tta_input_ta')
        || document.querySelector('textarea[aria-label="Text to translate"]');
    if (!input) {
      proxy.setFailed('Source input not found');
      return;
    }
    if (input.value === text) {
      console.log('using cached result');
      lastText = '';
      return;
    }
    input.value = text;
    input.dispatchEvent(new Event('input', { bubbles: true, cancelable: true }));
    return;
  }

  var url = 'https://www.bing.com/translator/?from=' + from + '&to=' + to +
            '&text=' + encodeURIComponent(text);
  console.log('setting url', url);
  window.location = url;
}

function init() {
  proxy.translate.connect(translate);
  setInterval(checkFinished, 300);
}