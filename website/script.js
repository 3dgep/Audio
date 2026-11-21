// Tabs for the code examples, and simple syntax colors for the code blocks.

// Tabs: click a tab, or use the arrow keys when a tab has focus.
for (const tabs of document.querySelectorAll('[data-tabs]')) {
    const buttons = [...tabs.querySelectorAll('[role="tab"]')];

    function select(button) {
        for (const other of buttons) {
            const selected = other === button;
            other.setAttribute('aria-selected', selected);
            other.tabIndex = selected ? 0 : -1;
            document.getElementById(other.getAttribute('aria-controls')).hidden = !selected;
        }
    }

    for (const [index, button] of buttons.entries()) {
        button.addEventListener('click', () => select(button));
        button.addEventListener('keydown', (event) => {
            const step = { ArrowRight: 1, ArrowLeft: -1 }[event.key];
            if (step) {
                const next = buttons[(index + step + buttons.length) % buttons.length];
                select(next);
                next.focus();
                event.preventDefault();
            }
        });
    }
}

// Syntax colors for C, C++, CMake and shell code.
const keywords = {
    cpp: 'auto bool break case char class const constexpr continue default delete do double else enum explicit false float for if int long namespace new nullptr private public return static struct switch this true using void while',
    c: 'break case char const continue default do double else enum float for if int long return sizeof static struct switch void while NULL',
    cmake: 'include FetchContent_Declare FetchContent_MakeAvailable find_package target_link_libraries add_executable list',
    sh: 'vcpkg install',
};

// Each rule is a token type and a regular expression. In C and C++, comments start
// with // or /*; in CMake and shell scripts, they start with #.
const cRules = [
    ['comment', /\/\/.*|\/\*[\s\S]*?\*\//],
    ['preprocessor', /#\w+.*/],
    ['string', /"(?:[^"\\\n]|\\.)*"/],
    ['number', /\b\d+(?:\.\d+)?f?\b|\b\d+s\b/],
    ['type', /\b(?:Audio|std)(?:::\w+)+|\bAUDIO_\w+\b/],
    ['word', /\b[A-Za-z_]\w*\b/],
];
const scriptRules = [
    ['comment', /#.*/],
    ['string', /"(?:[^"\\\n]|\\.)*"/],
    ['word', /\b[A-Za-z_]\w*\b/],
];

function highlight(html, lang) {
    const words = new Set((keywords[lang] || '').split(' '));
    const rules = lang === 'c' || lang === 'cpp' ? cRules : scriptRules;
    let output = '';
    let rest = html;

    // Repeatedly find the earliest match of any rule.
    while (rest.length > 0) {
        let best = null;
        for (const [name, regex] of rules) {
            const match = regex.exec(rest);
            if (match && (!best || match.index < best.match.index)) best = { name, match };
        }
        if (!best) {
            output += rest;
            break;
        }

        const { name, match } = best;
        output += rest.slice(0, match.index);
        const text = match[0];
        if (name === 'word') {
            output += words.has(text) ? `<span class="tok-keyword">${text}</span>` : text;
        } else {
            output += `<span class="tok-${name}">${text}</span>`;
        }
        rest = rest.slice(match.index + text.length);
    }
    return output;
}

for (const block of document.querySelectorAll('pre.code[data-lang] code')) {
    block.innerHTML = highlight(block.innerHTML, block.parentElement.dataset.lang);
}
