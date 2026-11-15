let hideTimer = null;

/** Show a short, non-blocking status message at the bottom of the canvas. */
export function toast(message, { warn = false, duration = 3000 } = {}) {
    const el = document.getElementById('toast');
    el.textContent = message;
    el.classList.toggle('warn', warn);
    el.classList.add('visible');
    clearTimeout(hideTimer);
    hideTimer = setTimeout(() => el.classList.remove('visible'), duration);
}
