var lastText = '';
var active = window.location.href !== "about:blank";

function findSource() {
  return document.querySelector('textarea.er8xn')
      || document.querySelector('textarea[aria-label="Source text"]')
      || document.querySelector('textarea[aria-label="Search languages"]');
}

function findTarget() {
  var spans = document.querySelectorAll('span.HwtZe > span > span');
  if (spans.length) return Array.from(spans).map(function (s) {
    return s.innerText;
  }).join(' ');

  var data = document.querySelector('[data-result="translation"]');
  if (data) return data.innerText.trim();

  return '';
}

function checkFinished() {
  if (!active) return;

  var text = findTarget().trim();
  if (text === lastText || text === '')
    return;

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

  if (window.location.href.indexOf('//translate.google') !== -1
      && window.location.href.indexOf('&tl=' + to + '&') !== -1) {
    var input = findSource();
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

  var url = 'https://translate.google.com/?sl=auto&tl=' + to +
            '&text=' + encodeURIComponent(text) + '&op=translate';
  console.log('setting url', url);
  window.location = url;
}

function init() {
  proxy.translate.connect(translate);
  setInterval(checkFinished, 300);
}