var lastText = '';
var active = window.location.href !== 'about:blank';

function findTarget() {
  var sels = [
    'd-textarea[data-testid=translator-target-input] p',
    'd-textarea.lmt__target_textarea p',
    'div#target-dummydiv'
  ];
  for (var i = 0; i < sels.length; ++i) {
    var el = document.querySelector(sels[i]);
    if (el) {
      return (el.innerText || el.innerHTML || '').trim();
    }
  }
  return '';
}

function findSource() {
  var sels = [
    'd-textarea[dl-test=translator-source-input] p',
    'd-textarea[data-testid=translator-source-input] p',
    'd-textarea.lmt__source_textarea p'
  ];
  for (var i = 0; i < sels.length; ++i) {
    var el = document.querySelector(sels[i]);
    if (el) return el;
  }
  return null;
}

function checkFinished() {
  if (!active) return;
  var text = findTarget();
  if (text === lastText || text === '') return;
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

  from = (from === 'zh-CN' || from === 'zh-TW') ? 'zh' : from;
  to = (to === 'zh-CN' || to === 'zh-TW') ? 'zh' : to;

  var supported = ['ru','en','de','fr','es','pt','it','nl','pl','ja','zh',
    'uk','bg','hu','el','da','id','lt','ro','sk','tr','fi','cs','sv','et','ko','ar'];
  if (supported.indexOf(from) === -1) {
    proxy.setFailed('Source language not supported');
    return;
  }
  if (supported.indexOf(to) === -1) {
    proxy.setFailed('Target language not supported');
    return;
  }

  active = true;
  var single = text.replace(/(?:\r\n|\r|\n)/g, ' ');
  var langs = from + '/' + to + '/';

  if (window.location.href.indexOf('www.deepl.com/translator') !== -1
      && window.location.href.indexOf(langs) !== -1) {
    var input = findSource();
    if (!input) {
      proxy.setFailed('Source input not found');
      return;
    }
    if (input.innerText === single) {
      console.log('using cached result');
      lastText = '';
      return;
    }
    input.innerText = single;
    var dup = document.querySelector('div#source-dummydiv');
    if (dup) dup.innerHTML = single;
    setTimeout(function () {
      input.dispatchEvent(new Event('input', { bubbles: true, cancelable: true }));
    }, 300);
    return;
  }

  var url = 'https://www.deepl.com/translator#' + langs + encodeURIComponent(single);
  console.log('setting url', url);
  window.location = url;
}

function init() {
  proxy.translate.connect(translate);
  setInterval(checkFinished, 300);
}