#!/usr/bin/env node
/* teb2 browser worker — Playwright automation subprocess. ≤166 lines */
'use strict';

const readline = require('readline');

async function runAction(page, action, target, value) {
    switch (action) {
    case 'navigate':
        await page.goto(target, { waitUntil: 'domcontentloaded', timeout: 15000 });
        return { ok: true, data: page.url() };
    case 'click':
        await page.click(target, { timeout: 5000 });
        return { ok: true, data: 'clicked' };
    case 'type':
        await page.fill(target, value || '');
        return { ok: true, data: 'typed' };
    case 'screenshot': {
        const buf = await page.screenshot({ type: 'png' });
        return { ok: true, data: 'png:' + buf.length };
    }
    case 'extract_text': {
        const text = await page.textContent(target);
        return { ok: true, data: (text || '').slice(0, 1024) };
    }
    default:
        return { ok: false, data: 'unknown_action:' + action };
    }
}

async function main() {
    let browser = null;
    let page = null;

    try {
        const pw = require('playwright');
        browser = await pw.chromium.launch({ headless: true });
        page = await browser.newPage();
    } catch (e) {
        browser = null;
        page = null;
    }

    const rl = readline.createInterface({
        input: process.stdin,
        crlfDelay: Infinity
    });

    for await (const line of rl) {
        const trimmed = line.trim();
        if (!trimmed) continue;

        const parts = trimmed.split('\t');
        const action = parts[0] || '';
        const target = parts[1] || '';
        const value  = parts[2] || '';
        let result;

        if (!page) {
            result = { ok: false, data: 'playwright_unavailable' };
        } else {
            try {
                result = await runAction(page, action, target, value);
            } catch (e) {
                result = { ok: false, data: String(e.message || e).slice(0, 256) };
            }
        }

        process.stdout.write(JSON.stringify(result) + '\n');
    }

    if (browser) {
        try { await browser.close(); } catch (_) { /* ignore */ }
    }
}

main().catch(function (e) {
    process.stderr.write(String(e) + '\n');
    process.exit(1);
});
