// include: shell.js
// include: minimum_runtime_check.js
(function() {
  // "30.0.0" -> 300000
  function humanReadableVersionToPacked(str) {
    str = str.split('-')[0]; // Remove any trailing part from e.g. "12.53.3-alpha"
    var vers = str.split('.').slice(0, 3);
    while(vers.length < 3) vers.push('00');
    vers = vers.map((n, i, arr) => n.padStart(2, '0'));
    return vers.join('');
  }
  // 300000 -> "30.0.0"
  var packedVersionToHumanReadable = n => [n / 10000 | 0, (n / 100 | 0) % 100, n % 100].join('.');

  var TARGET_NOT_SUPPORTED = 2147483647;

  // Note: We use a typeof check here instead of optional chaining using
  // globalThis because older browsers might not have globalThis defined.
  var currentNodeVersion = typeof process !== 'undefined' && process.versions?.node ? humanReadableVersionToPacked(process.versions.node) : TARGET_NOT_SUPPORTED;
  if (currentNodeVersion < 180300) {
    throw new Error(`This emscripten-generated code requires node v${ packedVersionToHumanReadable(180300) } (detected v${packedVersionToHumanReadable(currentNodeVersion)})`);
  }

  var userAgent = typeof navigator !== 'undefined' && navigator.userAgent;
  if (!userAgent) {
    return;
  }

  var currentSafariVersion = userAgent.includes("Safari/") && !userAgent.includes("Chrome/") && userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/) ? humanReadableVersionToPacked(userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentSafariVersion < 150000) {
    throw new Error(`This emscripten-generated code requires Safari v${ packedVersionToHumanReadable(150000) } (detected v${currentSafariVersion})`);
  }

  var currentFirefoxVersion = userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentFirefoxVersion < 79) {
    throw new Error(`This emscripten-generated code requires Firefox v79 (detected v${currentFirefoxVersion})`);
  }

  var currentChromeVersion = userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentChromeVersion < 85) {
    throw new Error(`This emscripten-generated code requires Chrome v85 (detected v${currentChromeVersion})`);
  }
})();

// end include: minimum_runtime_check.js
// The Module object: Our interface to the outside world. We import
// and export values on it. There are various ways Module can be used:
// 1. Not defined. We create it here
// 2. A function parameter, function(moduleArg) => Promise<Module>
// 3. pre-run appended it, var Module = {}; ..generated code..
// 4. External script tag defines var Module.
// We need to check if Module already exists (e.g. case 3 above).
// Substitution will be replaced with actual code on later stage of the build,
// this way Closure Compiler will not mangle it (e.g. case 4. above).
// Note that if you want to run closure, and also to use Module
// after the generated code, you will need to define   var Module = {};
// before the code. Then that object will be used in the code, and you
// can continue to use Module afterwards as well.
var Module = typeof Module != 'undefined' ? Module : {};

// Determine the runtime environment we are in. You can customize this by
// setting the ENVIRONMENT setting at compile time (see settings.js).

// Attempt to auto-detect the environment
var ENVIRONMENT_IS_WEB = !!globalThis.window;
var ENVIRONMENT_IS_WORKER = !!globalThis.WorkerGlobalScope;
// N.b. Electron.js environment is simultaneously a NODE-environment, but
// also a web environment.
var ENVIRONMENT_IS_NODE = globalThis.process?.versions?.node && globalThis.process?.type != 'renderer';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;

// --pre-jses are emitted after the Module integration code, so that they can
// refer to Module (if they choose; they can also define Module)


var arguments_ = [];
var thisProgram = './this.program';
var quit_ = (status, toThrow) => {
  throw toThrow;
};

// In MODULARIZE mode _scriptName needs to be captured already at the very top of the page immediately when the page is parsed, so it is generated there
// before the page load. In non-MODULARIZE modes generate it here.
var _scriptName = globalThis.document?.currentScript?.src;

if (typeof __filename != 'undefined') { // Node
  _scriptName = __filename;
} else
if (ENVIRONMENT_IS_WORKER) {
  _scriptName = self.location.href;
}

// `/` should be present at the end if `scriptDirectory` is not empty
var scriptDirectory = '';
function locateFile(path) {
  if (Module['locateFile']) {
    return Module['locateFile'](path, scriptDirectory);
  }
  return scriptDirectory + path;
}

// Hooks that are implemented differently in different runtime environments.
var readAsync, readBinary;

if (ENVIRONMENT_IS_NODE) {
  const isNode = globalThis.process?.versions?.node && globalThis.process?.type != 'renderer';
  if (!isNode) throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');

  // These modules will usually be used on Node.js. Load them eagerly to avoid
  // the complexity of lazy-loading.
  var fs = require('node:fs');

  scriptDirectory = __dirname + '/';

// include: node_shell_read.js
readBinary = (filename) => {
  // We need to re-wrap `file://` strings to URLs.
  filename = isFileURI(filename) ? new URL(filename) : filename;
  var ret = fs.readFileSync(filename);
  assert(Buffer.isBuffer(ret));
  return ret;
};

readAsync = async (filename, binary = true) => {
  // See the comment in the `readBinary` function.
  filename = isFileURI(filename) ? new URL(filename) : filename;
  var ret = fs.readFileSync(filename, binary ? undefined : 'utf8');
  assert(binary ? Buffer.isBuffer(ret) : typeof ret == 'string');
  return ret;
};
// end include: node_shell_read.js
  if (process.argv.length > 1) {
    thisProgram = process.argv[1].replace(/\\/g, '/');
  }

  arguments_ = process.argv.slice(2);

  // MODULARIZE will export the module in the proper place outside, we don't need to export here
  if (typeof module != 'undefined') {
    module['exports'] = Module;
  }

  quit_ = (status, toThrow) => {
    process.exitCode = status;
    throw toThrow;
  };

} else
if (ENVIRONMENT_IS_SHELL) {

} else

// Note that this includes Node.js workers when relevant (pthreads is enabled).
// Node.js workers are detected as a combination of ENVIRONMENT_IS_WORKER and
// ENVIRONMENT_IS_NODE.
if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  try {
    scriptDirectory = new URL('.', _scriptName).href; // includes trailing slash
  } catch {
    // Must be a `blob:` or `data:` URL (e.g. `blob:http://site.com/etc/etc`), we cannot
    // infer anything from them.
  }

  if (!(globalThis.window || globalThis.WorkerGlobalScope)) throw new Error('not compiled for this environment (did you build to HTML and try to run it not on the web, or set ENVIRONMENT to something - like node - and run it someplace else - like on the web?)');

  {
// include: web_or_worker_shell_read.js
if (ENVIRONMENT_IS_WORKER) {
    readBinary = (url) => {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', url, false);
      xhr.responseType = 'arraybuffer';
      xhr.send(null);
      return new Uint8Array(/** @type{!ArrayBuffer} */(xhr.response));
    };
  }

  readAsync = async (url) => {
    // Fetch has some additional restrictions over XHR, like it can't be used on a file:// url.
    // See https://github.com/github/fetch/pull/92#issuecomment-140665932
    // Cordova or Electron apps are typically loaded from a file:// url.
    // So use XHR on webview if URL is a file URL.
    if (isFileURI(url)) {
      return new Promise((resolve, reject) => {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.responseType = 'arraybuffer';
        xhr.onload = () => {
          if (xhr.status == 200 || (xhr.status == 0 && xhr.response)) { // file URLs can return 0
            resolve(xhr.response);
            return;
          }
          reject(xhr.status);
        };
        xhr.onerror = reject;
        xhr.send(null);
      });
    }
    var response = await fetch(url, { credentials: 'same-origin' });
    if (response.ok) {
      return response.arrayBuffer();
    }
    throw new Error(response.status + ' : ' + response.url);
  };
// end include: web_or_worker_shell_read.js
  }
} else
{
  throw new Error('environment detection error');
}

var out = console.log.bind(console);
var err = console.error.bind(console);

var IDBFS = 'IDBFS is no longer included by default; build with -lidbfs.js';
var PROXYFS = 'PROXYFS is no longer included by default; build with -lproxyfs.js';
var WORKERFS = 'WORKERFS is no longer included by default; build with -lworkerfs.js';
var FETCHFS = 'FETCHFS is no longer included by default; build with -lfetchfs.js';
var ICASEFS = 'ICASEFS is no longer included by default; build with -licasefs.js';
var JSFILEFS = 'JSFILEFS is no longer included by default; build with -ljsfilefs.js';
var OPFS = 'OPFS is no longer included by default; build with -lopfs.js';

var NODEFS = 'NODEFS is no longer included by default; build with -lnodefs.js';

// perform assertions in shell.js after we set up out() and err(), as otherwise
// if an assertion fails it cannot print the message

assert(!ENVIRONMENT_IS_SHELL, 'shell environment detected but not enabled at build time.  Add `shell` to `-sENVIRONMENT` to enable.');

// end include: shell.js

// include: preamble.js
// === Preamble library stuff ===

// Documentation for the public APIs defined in this file must be updated in:
//    site/source/docs/api_reference/preamble.js.rst
// A prebuilt local version of the documentation is available at:
//    site/build/text/docs/api_reference/preamble.js.txt
// You can also build docs locally as HTML or other formats in site/
// An online HTML version (which may be of a different version of Emscripten)
//    is up at http://kripken.github.io/emscripten-site/docs/api_reference/preamble.js.html

var wasmBinary;

if (!globalThis.WebAssembly) {
  err('no native wasm support detected');
}

// Wasm globals

//========================================
// Runtime essentials
//========================================

// whether we are quitting the application. no code should run after this.
// set in exit() and abort()
var ABORT = false;

// set by exit() and abort().  Passed to 'onExit' handler.
// NOTE: This is also used as the process return code in shell environments
// but only when noExitRuntime is false.
var EXITSTATUS;

// In STRICT mode, we only define assert() when ASSERTIONS is set.  i.e. we
// don't define it at all in release modes.  This matches the behaviour of
// MINIMAL_RUNTIME.
// TODO(sbc): Make this the default even without STRICT enabled.
/** @type {function(*, string=)} */
function assert(condition, text) {
  if (!condition) {
    abort('Assertion failed' + (text ? ': ' + text : ''));
  }
}

// We used to include malloc/free by default in the past. Show a helpful error in
// builds with assertions.

/**
 * Indicates whether filename is delivered via file protocol (as opposed to http/https)
 * @noinline
 */
var isFileURI = (filename) => filename.startsWith('file://');

// include: runtime_common.js
// include: runtime_stack_check.js
// Initializes the stack cookie. Called at the startup of main and at the startup of each thread in pthreads mode.
function writeStackCookie() {
  var max = _emscripten_stack_get_end();
  assert((max & 3) == 0);
  // If the stack ends at address zero we write our cookies 4 bytes into the
  // stack.  This prevents interference with SAFE_HEAP and ASAN which also
  // monitor writes to address zero.
  if (max == 0) {
    max += 4;
  }
  // The stack grow downwards towards _emscripten_stack_get_end.
  // We write cookies to the final two words in the stack and detect if they are
  // ever overwritten.
  HEAPU32[((max)>>2)] = 0x02135467;
  HEAPU32[(((max)+(4))>>2)] = 0x89BACDFE;
  // Also test the global address 0 for integrity.
  HEAPU32[((0)>>2)] = 1668509029;
}

function checkStackCookie() {
  if (ABORT) return;
  var max = _emscripten_stack_get_end();
  // See writeStackCookie().
  if (max == 0) {
    max += 4;
  }
  var cookie1 = HEAPU32[((max)>>2)];
  var cookie2 = HEAPU32[(((max)+(4))>>2)];
  if (cookie1 != 0x02135467 || cookie2 != 0x89BACDFE) {
    abort(`Stack overflow! Stack cookie has been overwritten at ${ptrToString(max)}, expected hex dwords 0x89BACDFE and 0x2135467, but received ${ptrToString(cookie2)} ${ptrToString(cookie1)}`);
  }
  // Also test the global address 0 for integrity.
  if (HEAPU32[((0)>>2)] != 0x63736d65 /* 'emsc' */) {
    abort('Runtime error: The application has corrupted its heap memory area (address zero)!');
  }
}
// end include: runtime_stack_check.js
// include: runtime_exceptions.js
// Base Emscripten EH error class
class EmscriptenEH {}

class EmscriptenSjLj extends EmscriptenEH {}

// end include: runtime_exceptions.js
// include: runtime_debug.js
var runtimeDebug = true; // Switch to false at runtime to disable logging at the right times

// Used by XXXXX_DEBUG settings to output debug messages.
function dbg(...args) {
  if (!runtimeDebug && typeof runtimeDebug != 'undefined') return;
  // TODO(sbc): Make this configurable somehow.  Its not always convenient for
  // logging to show up as warnings.
  console.warn(...args);
}

// Endianness check
(() => {
  var h16 = new Int16Array(1);
  var h8 = new Int8Array(h16.buffer);
  h16[0] = 0x6373;
  if (h8[0] !== 0x73 || h8[1] !== 0x63) abort('Runtime error: expected the system to be little-endian! (Run with -sSUPPORT_BIG_ENDIAN to bypass)');
})();

function consumedModuleProp(prop) {
  if (!Object.getOwnPropertyDescriptor(Module, prop)) {
    Object.defineProperty(Module, prop, {
      configurable: true,
      set() {
        abort(`Attempt to set \`Module.${prop}\` after it has already been processed.  This can happen, for example, when code is injected via '--post-js' rather than '--pre-js'`);

      }
    });
  }
}

function makeInvalidEarlyAccess(name) {
  return () => assert(false, `call to '${name}' via reference taken before Wasm module initialization`);

}

function ignoredModuleProp(prop) {
  if (Object.getOwnPropertyDescriptor(Module, prop)) {
    abort(`\`Module.${prop}\` was supplied but \`${prop}\` not included in INCOMING_MODULE_JS_API`);
  }
}

// forcing the filesystem exports a few things by default
function isExportedByForceFilesystem(name) {
  return name === 'FS_createPath' ||
         name === 'FS_createDataFile' ||
         name === 'FS_createPreloadedFile' ||
         name === 'FS_preloadFile' ||
         name === 'FS_unlink' ||
         name === 'addRunDependency' ||
         // The old FS has some functionality that WasmFS lacks.
         name === 'FS_createLazyFile' ||
         name === 'FS_createDevice' ||
         name === 'removeRunDependency';
}

/**
 * Intercept access to a symbols in the global symbol.  This enables us to give
 * informative warnings/errors when folks attempt to use symbols they did not
 * include in their build, or no symbols that no longer exist.
 *
 * We don't define this in MODULARIZE mode since in that mode emscripten symbols
 * are never placed in the global scope.
 */
function hookGlobalSymbolAccess(sym, func) {
  if (!Object.getOwnPropertyDescriptor(globalThis, sym)) {
    Object.defineProperty(globalThis, sym, {
      configurable: true,
      get() {
        func();
        return undefined;
      }
    });
  }
}

function missingGlobal(sym, msg) {
  hookGlobalSymbolAccess(sym, () => {
    warnOnce(`\`${sym}\` is no longer defined by emscripten. ${msg}`);
  });
}

missingGlobal('buffer', 'Please use HEAP8.buffer or wasmMemory.buffer');
missingGlobal('asm', 'Please use wasmExports instead');

function missingLibrarySymbol(sym) {
  hookGlobalSymbolAccess(sym, () => {
    // Can't `abort()` here because it would break code that does runtime
    // checks.  e.g. `if (typeof SDL === 'undefined')`.
    var msg = `\`${sym}\` is a library symbol and not included by default; add it to your library.js __deps or to DEFAULT_LIBRARY_FUNCS_TO_INCLUDE on the command line`;
    // DEFAULT_LIBRARY_FUNCS_TO_INCLUDE requires the name as it appears in
    // library.js, which means $name for a JS name with no prefix, or name
    // for a JS name like _name.
    var librarySymbol = sym;
    if (!librarySymbol.startsWith('_')) {
      librarySymbol = '$' + sym;
    }
    msg += ` (e.g. -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='${librarySymbol}')`;
    if (isExportedByForceFilesystem(sym)) {
      msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
    }
    warnOnce(msg);
  });

  // Any symbol that is not included from the JS library is also (by definition)
  // not exported on the Module object.
  unexportedRuntimeSymbol(sym);
}

function unexportedRuntimeSymbol(sym) {
  if (!Object.getOwnPropertyDescriptor(Module, sym)) {
    Object.defineProperty(Module, sym, {
      configurable: true,
      get() {
        var msg = `'${sym}' was not exported. add it to EXPORTED_RUNTIME_METHODS (see the Emscripten FAQ)`;
        if (isExportedByForceFilesystem(sym)) {
          msg += '. Alternatively, forcing filesystem support (-sFORCE_FILESYSTEM) can export this for you';
        }
        abort(msg);
      },
    });
  }
}

// end include: runtime_debug.js
// Memory management

var runtimeInitialized = false;



function updateMemoryViews() {
  var b = wasmMemory.buffer;
  HEAP8 = new Int8Array(b);
  HEAP16 = new Int16Array(b);
  HEAPU8 = new Uint8Array(b);
  HEAPU16 = new Uint16Array(b);
  HEAP32 = new Int32Array(b);
  HEAPU32 = new Uint32Array(b);
  HEAPF32 = new Float32Array(b);
  HEAPF64 = new Float64Array(b);
  HEAP64 = new BigInt64Array(b);
  HEAPU64 = new BigUint64Array(b);
}

// include: memoryprofiler.js
// end include: memoryprofiler.js
// end include: runtime_common.js
assert(globalThis.Int32Array && globalThis.Float64Array && Int32Array.prototype.subarray && Int32Array.prototype.set,
       'JS engine does not provide full typed array support');

function preRun() {
  if (Module['preRun']) {
    if (typeof Module['preRun'] == 'function') Module['preRun'] = [Module['preRun']];
    while (Module['preRun'].length) {
      addOnPreRun(Module['preRun'].shift());
    }
  }
  consumedModuleProp('preRun');
  // Begin ATPRERUNS hooks
  callRuntimeCallbacks(onPreRuns);
  // End ATPRERUNS hooks
}

function initRuntime() {
  assert(!runtimeInitialized);
  runtimeInitialized = true;

  checkStackCookie();

  // No ATINITS hooks

  wasmExports['__wasm_call_ctors']();

  // No ATPOSTCTORS hooks
}

function preMain() {
  checkStackCookie();
  // No ATMAINS hooks
}

function postRun() {
  checkStackCookie();
   // PThreads reuse the runtime from the main thread.

  if (Module['postRun']) {
    if (typeof Module['postRun'] == 'function') Module['postRun'] = [Module['postRun']];
    while (Module['postRun'].length) {
      addOnPostRun(Module['postRun'].shift());
    }
  }
  consumedModuleProp('postRun');

  // Begin ATPOSTRUNS hooks
  callRuntimeCallbacks(onPostRuns);
  // End ATPOSTRUNS hooks
}

/**
 * @param {string|number=} what
 */
function abort(what) {
  Module['onAbort']?.(what);

  what = `Aborted(${what})`;
  // TODO(sbc): Should we remove printing and leave it up to whoever
  // catches the exception?
  err(what);

  ABORT = true;

  if (what.search(/RuntimeError: [Uu]nreachable/) >= 0) {
    what += '. "unreachable" may be due to ASYNCIFY_STACK_SIZE not being large enough (try increasing it)';
  }

  // Use a wasm runtime error, because a JS error might be seen as a foreign
  // exception, which means we'd run destructors on it. We need the error to
  // simply make the program stop.
  // FIXME This approach does not work in Wasm EH because it currently does not assume
  // all RuntimeErrors are from traps; it decides whether a RuntimeError is from
  // a trap or not based on a hidden field within the object. So at the moment
  // we don't have a way of throwing a wasm trap from JS. TODO Make a JS API that
  // allows this in the wasm spec.

  // Suppress closure compiler warning here. Closure compiler's builtin extern
  // definition for WebAssembly.RuntimeError claims it takes no arguments even
  // though it can.
  // TODO(https://github.com/google/closure-compiler/pull/3913): Remove if/when upstream closure gets fixed.
  /** @suppress {checkTypes} */
  var e = new WebAssembly.RuntimeError(what);

  // Throw the error whether or not MODULARIZE is set because abort is used
  // in code paths apart from instantiation where an exception is expected
  // to be thrown when abort is called.
  throw e;
}

// show errors on likely calls to FS when it was not included
function fsMissing() {
  abort('Filesystem support (FS) was not included. The problem is that you are using files from JS, but files were not used from C/C++, so filesystem support was not auto-included. You can force-include filesystem support with -sFORCE_FILESYSTEM');
}
var FS = {
  init: fsMissing,
  createDataFile: fsMissing,
  createPreloadedFile: fsMissing,
  createLazyFile: fsMissing,
  open: fsMissing,
  mkdev: fsMissing,
  registerDevice:  fsMissing,
  analyzePath: fsMissing,
  ErrnoError: fsMissing,
};


function createExportWrapper(name, nargs) {
  return (...args) => {
    assert(runtimeInitialized, `native function \`${name}\` called before runtime initialization`);
    var f = wasmExports[name];
    assert(f, `exported native function \`${name}\` not found`);
    // Only assert for too many arguments. Too few can be valid since the missing arguments will be zero filled.
    assert(args.length <= nargs, `native function \`${name}\` called with ${args.length} args but expects ${nargs}`);
    return f(...args);
  };
}

var wasmBinaryFile;

function findWasmBinary() {
  return locateFile('mira_web.wasm');
}

function getBinarySync(file) {
  if (file == wasmBinaryFile && wasmBinary) {
    return new Uint8Array(wasmBinary);
  }
  if (readBinary) {
    return readBinary(file);
  }
  // Throwing a plain string here, even though it not normally advisable since
  // this gets turning into an `abort` in instantiateArrayBuffer.
  throw 'both async and sync fetching of the wasm failed';
}

async function getWasmBinary(binaryFile) {
  // If we don't have the binary yet, load it asynchronously using readAsync.
  if (!wasmBinary) {
    // Fetch the binary using readAsync
    try {
      var response = await readAsync(binaryFile);
      return new Uint8Array(response);
    } catch {
      // Fall back to getBinarySync below;
    }
  }

  // Otherwise, getBinarySync should be able to get it synchronously
  return getBinarySync(binaryFile);
}

async function instantiateArrayBuffer(binaryFile, imports) {
  try {
    var binary = await getWasmBinary(binaryFile);
    var instance = await WebAssembly.instantiate(binary, imports);
    return instance;
  } catch (reason) {
    err(`failed to asynchronously prepare wasm: ${reason}`);

    // Warn on some common problems.
    if (isFileURI(binaryFile)) {
      err(`warning: Loading from a file URI (${binaryFile}) is not supported in most browsers. See https://emscripten.org/docs/getting_started/FAQ.html#how-do-i-run-a-local-webserver-for-testing-why-does-my-program-stall-in-downloading-or-preparing`);
    }
    abort(reason);
  }
}

async function instantiateAsync(binary, binaryFile, imports) {
  if (!binary
      // Don't use streaming for file:// delivered objects in a webview, fetch them synchronously.
      && !isFileURI(binaryFile)
      // Avoid instantiateStreaming() on Node.js environment for now, as while
      // Node.js v18.1.0 implements it, it does not have a full fetch()
      // implementation yet.
      //
      // Reference:
      //   https://github.com/emscripten-core/emscripten/pull/16917
      && !ENVIRONMENT_IS_NODE
     ) {
    try {
      var response = fetch(binaryFile, { credentials: 'same-origin' });
      var instantiationResult = await WebAssembly.instantiateStreaming(response, imports);
      return instantiationResult;
    } catch (reason) {
      // We expect the most common failure cause to be a bad MIME type for the binary,
      // in which case falling back to ArrayBuffer instantiation should work.
      err(`wasm streaming compile failed: ${reason}`);
      err('falling back to ArrayBuffer instantiation');
      // fall back of instantiateArrayBuffer below
    };
  }
  return instantiateArrayBuffer(binaryFile, imports);
}

function getWasmImports() {
  // instrumenting imports is used in asyncify in two ways: to add assertions
  // that check for proper import use, and for JSPI we use them to set up
  // the Promise API on the import side.
  Asyncify.instrumentWasmImports(wasmImports);
  // prepare imports
  var imports = {
    'env': wasmImports,
    'wasi_snapshot_preview1': wasmImports,
  };
  return imports;
}

// Create the wasm instance.
// Receives the wasm imports, returns the exports.
async function createWasm() {
  // Load the wasm module and create an instance of using native support in the JS engine.
  // handle a generated wasm instance, receiving its exports and
  // performing other necessary setup
  /** @param {WebAssembly.Module=} module*/
  function receiveInstance(instance, module) {
    wasmExports = instance.exports;

    wasmExports = Asyncify.instrumentWasmExports(wasmExports);

    assignWasmExports(wasmExports);

    updateMemoryViews();

    removeRunDependency('wasm-instantiate');
    return wasmExports;
  }
  addRunDependency('wasm-instantiate');

  // Prefer streaming instantiation if available.
  // Async compilation can be confusing when an error on the page overwrites Module
  // (for example, if the order of elements is wrong, and the one defining Module is
  // later), so we save Module and check it later.
  var trueModule = Module;
  function receiveInstantiationResult(result) {
    // 'result' is a ResultObject object which has both the module and instance.
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    assert(Module === trueModule, 'the Module object should not be replaced during async compilation - perhaps the order of HTML elements is wrong?');
    trueModule = null;
    // TODO: Due to Closure regression https://github.com/google/closure-compiler/issues/3193, the above line no longer optimizes out down to the following line.
    // When the regression is fixed, can restore the above PTHREADS-enabled path.
    return receiveInstance(result['instance']);
  }

  var info = getWasmImports();

  // User shell pages can write their own Module.instantiateWasm = function(imports, successCallback) callback
  // to manually instantiate the Wasm module themselves. This allows pages to
  // run the instantiation parallel to any other async startup actions they are
  // performing.
  // Also pthreads and wasm workers initialize the wasm instance through this
  // path.
  if (Module['instantiateWasm']) {
    return new Promise((resolve, reject) => {
      try {
        Module['instantiateWasm'](info, (inst, mod) => {
          resolve(receiveInstance(inst, mod));
        });
      } catch(e) {
        err(`Module.instantiateWasm callback failed with error: ${e}`);
        reject(e);
      }
    });
  }

  wasmBinaryFile ??= findWasmBinary();
  var result = await instantiateAsync(wasmBinary, wasmBinaryFile, info);
  var exports = receiveInstantiationResult(result);
  return exports;
}

// end include: preamble.js

// Begin JS library code


  class ExitStatus {
      name = 'ExitStatus';
      constructor(status) {
        this.message = `Program terminated with exit(${status})`;
        this.status = status;
      }
    }

  /** @type {!Int16Array} */
  var HEAP16;

  /** @type {!Int32Array} */
  var HEAP32;

  /** not-@type {!BigInt64Array} */
  var HEAP64;

  /** @type {!Int8Array} */
  var HEAP8;

  /** @type {!Float32Array} */
  var HEAPF32;

  /** @type {!Float64Array} */
  var HEAPF64;

  /** @type {!Uint16Array} */
  var HEAPU16;

  /** @type {!Uint32Array} */
  var HEAPU32;

  /** not-@type {!BigUint64Array} */
  var HEAPU64;

  /** @type {!Uint8Array} */
  var HEAPU8;

  var callRuntimeCallbacks = (callbacks) => {
      while (callbacks.length > 0) {
        // Pass the module as the first argument.
        callbacks.shift()(Module);
      }
    };
  var onPostRuns = [];
  var addOnPostRun = (cb) => onPostRuns.push(cb);

  var onPreRuns = [];
  var addOnPreRun = (cb) => onPreRuns.push(cb);

  var runDependencies = 0;
  
  
  var dependenciesFulfilled = null;
  
  var runDependencyTracking = {
  };
  
  var runDependencyWatcher = null;
  var removeRunDependency = (id) => {
      runDependencies--;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'removeRunDependency requires an ID');
      assert(runDependencyTracking[id]);
      delete runDependencyTracking[id];
      if (runDependencies == 0) {
        if (runDependencyWatcher !== null) {
          clearInterval(runDependencyWatcher);
          runDependencyWatcher = null;
        }
        if (dependenciesFulfilled) {
          var callback = dependenciesFulfilled;
          dependenciesFulfilled = null;
          callback(); // can add another dependenciesFulfilled
        }
      }
    };
  
  
  var addRunDependency = (id) => {
      runDependencies++;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      assert(id, 'addRunDependency requires an ID')
      assert(!runDependencyTracking[id]);
      runDependencyTracking[id] = 1;
      if (runDependencyWatcher === null && globalThis.setInterval) {
        // Check for missing dependencies every few seconds
        runDependencyWatcher = setInterval(() => {
          if (ABORT) {
            clearInterval(runDependencyWatcher);
            runDependencyWatcher = null;
            return;
          }
          var shown = false;
          for (var dep in runDependencyTracking) {
            if (!shown) {
              shown = true;
              err('still waiting on run dependencies:');
            }
            err(`dependency: ${dep}`);
          }
          if (shown) {
            err('(end of list)');
          }
        }, 10000);
        // Prevent this timer from keeping the runtime alive if nothing
        // else is.
        runDependencyWatcher.unref?.()
      }
    };


  var dynCalls = {
  };
  var dynCallLegacy = (sig, ptr, args) => {
      sig = sig.replace(/p/g, 'i')
      assert(sig in dynCalls, `bad function pointer type - sig is not in dynCalls: '${sig}'`);
      if (args?.length) {
        // j (64-bit integer) is fine, and is implemented as a BigInt. Without
        // legalization, the number of parameters should match (j is not expanded
        // into two i's).
        assert(args.length === sig.length - 1);
      } else {
        assert(sig.length == 1);
      }
      var f = dynCalls[sig];
      return f(ptr, ...args);
    };
  var dynCall = (sig, ptr, args = [], promising = false) => {
      assert(ptr, `null function pointer in dynCall`);
      assert(!promising, 'async dynCall is not supported in this mode')
      var rtn = dynCallLegacy(sig, ptr, args);
  
      function convert(rtn) {
        return rtn;
      }
  
      return convert(rtn);
    };

  
    /**
   * @param {number} ptr
   * @param {string} type
   */
  function getValue(ptr, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': return HEAP8[ptr];
      case 'i8': return HEAP8[ptr];
      case 'i16': return HEAP16[((ptr)>>1)];
      case 'i32': return HEAP32[((ptr)>>2)];
      case 'i64': return HEAP64[((ptr)>>3)];
      case 'float': return HEAPF32[((ptr)>>2)];
      case 'double': return HEAPF64[((ptr)>>3)];
      case '*': return HEAPU32[((ptr)>>2)];
      default: abort(`invalid type for getValue: ${type}`);
    }
  }

  var noExitRuntime = true;

  function ptrToString(ptr) {
      assert(typeof ptr === 'number', `ptrToString expects a number, got ${typeof ptr}`);
      // Convert to 32-bit unsigned value
      ptr >>>= 0;
      return '0x' + ptr.toString(16).padStart(8, '0');
    }


  
    /**
   * @param {number} ptr
   * @param {number} value
   * @param {string} type
   */
  function setValue(ptr, value, type = 'i8') {
    if (type.endsWith('*')) type = '*';
    switch (type) {
      case 'i1': HEAP8[ptr] = value; break;
      case 'i8': HEAP8[ptr] = value; break;
      case 'i16': HEAP16[((ptr)>>1)] = value; break;
      case 'i32': HEAP32[((ptr)>>2)] = value; break;
      case 'i64': HEAP64[((ptr)>>3)] = BigInt(value); break;
      case 'float': HEAPF32[((ptr)>>2)] = value; break;
      case 'double': HEAPF64[((ptr)>>3)] = value; break;
      case '*': HEAPU32[((ptr)>>2)] = value; break;
      default: abort(`invalid type for setValue: ${type}`);
    }
  }

  var stackRestore = (val) => __emscripten_stack_restore(val);

  var stackSave = () => _emscripten_stack_get_current();

  var warnOnce = (text) => {
      warnOnce.shown ||= {};
      if (!warnOnce.shown[text]) {
        warnOnce.shown[text] = 1;
        if (ENVIRONMENT_IS_NODE) text = 'warning: ' + text;
        err(text);
      }
    };

  

  var __abort_js = () =>
      abort('native code called abort()');

  var UTF8Decoder = globalThis.TextDecoder && new TextDecoder();
  
  var findStringEnd = (heapOrArray, idx, maxBytesToRead, ignoreNul) => {
      var maxIdx = idx + maxBytesToRead;
      if (ignoreNul) return maxIdx;
      // TextDecoder needs to know the byte length in advance, it doesn't stop on
      // null terminator by itself.
      // As a tiny code save trick, compare idx against maxIdx using a negation,
      // so that maxBytesToRead=undefined/NaN means Infinity.
      while (heapOrArray[idx] && !(idx >= maxIdx)) ++idx;
      return idx;
    };
  
  
    /**
   * Given a pointer 'idx' to a null-terminated UTF8-encoded string in the given
   * array that contains uint8 values, returns a copy of that string as a
   * Javascript String object.
   * heapOrArray is either a regular array, or a JavaScript typed array view.
   * @param {number=} idx
   * @param {number=} maxBytesToRead
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ArrayToString = (heapOrArray, idx = 0, maxBytesToRead, ignoreNul) => {
  
      var endPtr = findStringEnd(heapOrArray, idx, maxBytesToRead, ignoreNul);
  
      // When using conditional TextDecoder, skip it for short strings as the overhead of the native call is not worth it.
      if (endPtr - idx > 16 && heapOrArray.buffer && UTF8Decoder) {
        return UTF8Decoder.decode(heapOrArray.subarray(idx, endPtr));
      }
      var str = '';
      while (idx < endPtr) {
        // For UTF8 byte structure, see:
        // http://en.wikipedia.org/wiki/UTF-8#Description
        // https://www.ietf.org/rfc/rfc2279.txt
        // https://tools.ietf.org/html/rfc3629
        var u0 = heapOrArray[idx++];
        if (!(u0 & 0x80)) { str += String.fromCharCode(u0); continue; }
        var u1 = heapOrArray[idx++] & 63;
        if ((u0 & 0xE0) == 0xC0) { str += String.fromCharCode(((u0 & 31) << 6) | u1); continue; }
        var u2 = heapOrArray[idx++] & 63;
        if ((u0 & 0xF0) == 0xE0) {
          u0 = ((u0 & 15) << 12) | (u1 << 6) | u2;
        } else {
          if ((u0 & 0xF8) != 0xF0) warnOnce(`Invalid UTF-8 leading byte ${ptrToString(u0)} encountered when deserializing a UTF-8 string in wasm memory to a JS string!`);
          u0 = ((u0 & 7) << 18) | (u1 << 12) | (u2 << 6) | (heapOrArray[idx++] & 63);
        }
  
        if (u0 < 0x10000) {
          str += String.fromCharCode(u0);
        } else {
          var ch = u0 - 0x10000;
          str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
        }
      }
      return str;
    };
  
    /**
   * Given a pointer 'ptr' to a null-terminated UTF8-encoded string in the
   * emscripten HEAP, returns a copy of that string as a Javascript String object.
   *
   * @param {number} ptr
   * @param {number=} maxBytesToRead - An optional length that specifies the
   *   maximum number of bytes to read. You can omit this parameter to scan the
   *   string until the first 0 byte. If maxBytesToRead is passed, and the string
   *   at [ptr, ptr+maxBytesToReadr[ contains a null byte in the middle, then the
   *   string will cut short at that byte index.
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ToString = (ptr, maxBytesToRead, ignoreNul) => {
      assert(typeof ptr == 'number', `UTF8ToString expects a number (got ${typeof ptr})`);
      return ptr ? UTF8ArrayToString(HEAPU8, ptr, maxBytesToRead, ignoreNul) : '';
    };
  var _emscripten_err = (str) => err(UTF8ToString(str));

  var _emscripten_get_device_pixel_ratio = () => {
      return globalThis.devicePixelRatio ?? 1.0;
    };

  var maybeCStringToJsString = (cString) => {
      // "cString > 2" checks if the input is a number, and isn't of the special
      // values we accept here, EMSCRIPTEN_EVENT_TARGET_* (which map to 0, 1, 2).
      // In other words, if cString > 2 then it's a pointer to a valid place in
      // memory, and points to a C string.
      return cString > 2 ? UTF8ToString(cString) : cString;
    };
  
  /** @type {Object} */
  var specialHTMLTargets = [0, globalThis.document ?? 0, globalThis.window ?? 0];
  var findEventTarget = (target) => {
      target = maybeCStringToJsString(target);
      var domElement = specialHTMLTargets[target] || globalThis.document?.querySelector(target);
      return domElement;
    };
  
  var getBoundingClientRect = (e) => specialHTMLTargets.indexOf(e) < 0 ? e.getBoundingClientRect() : {'left':0,'top':0};
  var _emscripten_get_element_css_size = (target, width, height) => {
      target = findEventTarget(target);
      if (!target) return -4;
  
      var rect = getBoundingClientRect(target);
      HEAPF64[((width)>>3)] = rect.width;
      HEAPF64[((height)>>3)] = rect.height;
  
      return 0;
    };

  var _emscripten_has_asyncify = () => 1;

  var _emscripten_request_animation_frame_loop = (cb, userData) => {
      function tick(timeStamp) {
        if (((a1, a2) => dynCall_idi(cb, a1, a2))(timeStamp, userData)) {
          requestAnimationFrame(tick);
        }
      }
      return requestAnimationFrame(tick);
    };

  var getHeapMax = () =>
      // Stay one Wasm page short of 4GB: while e.g. Chrome is able to allocate
      // full 4GB Wasm memories, the size will wrap back to 0 bytes in Wasm side
      // for any code that deals with heap sizes, which would require special
      // casing all heap size related code to treat 0 specially.
      2147483648;
  
  var alignMemory = (size, alignment) => {
      assert(alignment, "alignment argument is required");
      return Math.ceil(size / alignment) * alignment;
    };
  
  var growMemory = (size) => {
      var oldHeapSize = wasmMemory.buffer.byteLength;
      var pages = ((size - oldHeapSize + 65535) / 65536) | 0;
      try {
        // round size grow request up to wasm page size (fixed 64KB per spec)
        wasmMemory.grow(pages); // .grow() takes a delta compared to the previous size
        updateMemoryViews();
        return 1 /*success*/;
      } catch(e) {
        err(`growMemory: Attempted to grow heap from ${oldHeapSize} bytes to ${size} bytes, but got error: ${e}`);
      }
      // implicit 0 return to save code size (caller will cast "undefined" into 0
      // anyhow)
    };
  var _emscripten_resize_heap = (requestedSize) => {
      var oldSize = HEAPU8.length;
      // With CAN_ADDRESS_2GB or MEMORY64, pointers are already unsigned.
      requestedSize >>>= 0;
      // With multithreaded builds, races can happen (another thread might increase the size
      // in between), so return a failure, and let the caller retry.
      assert(requestedSize > oldSize);
  
      // Memory resize rules:
      // 1.  Always increase heap size to at least the requested size, rounded up
      //     to next page multiple.
      // 2a. If MEMORY_GROWTH_LINEAR_STEP == -1, excessively resize the heap
      //     geometrically: increase the heap size according to
      //     MEMORY_GROWTH_GEOMETRIC_STEP factor (default +20%), At most
      //     overreserve by MEMORY_GROWTH_GEOMETRIC_CAP bytes (default 96MB).
      // 2b. If MEMORY_GROWTH_LINEAR_STEP != -1, excessively resize the heap
      //     linearly: increase the heap size by at least
      //     MEMORY_GROWTH_LINEAR_STEP bytes.
      // 3.  Max size for the heap is capped at 2048MB-WASM_PAGE_SIZE, or by
      //     MAXIMUM_MEMORY, or by ASAN limit, depending on which is smallest
      // 4.  If we were unable to allocate as much memory, it may be due to
      //     over-eager decision to excessively reserve due to (3) above.
      //     Hence if an allocation fails, cut down on the amount of excess
      //     growth, in an attempt to succeed to perform a smaller allocation.
  
      // A limit is set for how much we can grow. We should not exceed that
      // (the wasm binary specifies it, so if we tried, we'd fail anyhow).
      var maxHeapSize = getHeapMax();
      if (requestedSize > maxHeapSize) {
        err(`Cannot enlarge memory, requested ${requestedSize} bytes, but the limit is ${maxHeapSize} bytes!`);
        return false;
      }
  
      // Loop through potential heap size increases. If we attempt a too eager
      // reservation that fails, cut down on the attempted size and reserve a
      // smaller bump instead. (max 3 times, chosen somewhat arbitrarily)
      for (var cutDown = 1; cutDown <= 4; cutDown *= 2) {
        var overGrownHeapSize = oldSize * (1 + 0.2 / cutDown); // ensure geometric growth
        // but limit overreserving (default to capping at +96MB overgrowth at most)
        overGrownHeapSize = Math.min(overGrownHeapSize, requestedSize + 100663296 );
  
        var newSize = Math.min(maxHeapSize, alignMemory(Math.max(requestedSize, overGrownHeapSize), 65536));
  
        var replacement = growMemory(newSize);
        if (replacement) {
  
          return true;
        }
      }
      err(`Failed to grow the heap from ${oldSize} bytes to ${newSize} bytes, not enough memory!`);
      return false;
    };

  var _emscripten_run_script = (ptr) => {
      eval(UTF8ToString(ptr));
    };

  var findCanvasEventTarget = findEventTarget;
  var _emscripten_set_canvas_element_size = (target, width, height) => {
      var canvas = findCanvasEventTarget(target);
      if (!canvas) return -4;
      canvas.width = width;
      canvas.height = height;
      return 0;
    };

  var onExits = [];
  var addOnExit = (cb) => onExits.push(cb);
  var JSEvents = {
  removeAllEventListeners() {
        while (JSEvents.eventHandlers.length) {
          JSEvents._removeHandler(JSEvents.eventHandlers.length - 1);
        }
        JSEvents.deferredCalls = [];
      },
  inEventHandler:0,
  deferredCalls:[],
  deferCall(targetFunction, precedence, argsList) {
        function arraysHaveEqualContent(arrA, arrB) {
          if (arrA.length != arrB.length) return false;
  
          for (var i in arrA) {
            if (arrA[i] != arrB[i]) return false;
          }
          return true;
        }
        // Test if the given call was already queued, and if so, don't add it again.
        for (var call of JSEvents.deferredCalls) {
          if (call.targetFunction == targetFunction && arraysHaveEqualContent(call.argsList, argsList)) {
            return;
          }
        }
        JSEvents.deferredCalls.push({
          targetFunction,
          precedence,
          argsList
        });
  
        JSEvents.deferredCalls.sort((x,y) => x.precedence - y.precedence);
      },
  removeDeferredCalls(targetFunction) {
        JSEvents.deferredCalls = JSEvents.deferredCalls.filter((call) => call.targetFunction != targetFunction);
      },
  canPerformEventHandlerRequests() {
        if (navigator.userActivation) {
          // Verify against transient activation status from UserActivation API
          // whether it is possible to perform a request here without needing to defer. See
          // https://developer.mozilla.org/en-US/docs/Web/Security/User_activation#transient_activation
          // and https://caniuse.com/mdn-api_useractivation
          // At the time of writing, Firefox does not support this API: https://bugzil.la/1791079
          return navigator.userActivation.isActive;
        }
  
        return JSEvents.inEventHandler && JSEvents.currentEventHandler.allowsDeferredCalls;
      },
  runDeferredCalls() {
        if (!JSEvents.canPerformEventHandlerRequests()) {
          return;
        }
        var deferredCalls = JSEvents.deferredCalls;
        JSEvents.deferredCalls = [];
        for (var call of deferredCalls) {
          call.targetFunction(...call.argsList);
        }
      },
  eventHandlers:[],
  removeAllHandlersOnTarget:(target, eventTypeString) => {
        for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
          if (JSEvents.eventHandlers[i].target == target &&
            (!eventTypeString || eventTypeString == JSEvents.eventHandlers[i].eventTypeString)) {
             JSEvents._removeHandler(i--);
           }
        }
      },
  _removeHandler(i) {
        var h = JSEvents.eventHandlers[i];
        h.target.removeEventListener(h.eventTypeString, h.eventListenerFunc, h.useCapture);
        JSEvents.eventHandlers.splice(i, 1);
      },
  registerOrRemoveHandler(eventHandler) {
        if (!eventHandler.target) {
          err('registerOrRemoveHandler: the target element for event handler registration does not exist, when processing the following event handler registration:');
          console.dir(eventHandler);
          return -4;
        }
        if (eventHandler.callbackfunc) {
          eventHandler.eventListenerFunc = function(event) {
            // Increment nesting count for the event handler.
            ++JSEvents.inEventHandler;
            JSEvents.currentEventHandler = eventHandler;
            // Process any old deferred calls the user has placed.
            JSEvents.runDeferredCalls();
            // Process the actual event, calls back to user C code handler.
            eventHandler.handlerFunc(event);
            // Process any new deferred calls that were placed right now from this event handler.
            JSEvents.runDeferredCalls();
            // Out of event handler - restore nesting count.
            --JSEvents.inEventHandler;
          };
  
          eventHandler.target.addEventListener(eventHandler.eventTypeString,
                                               eventHandler.eventListenerFunc,
                                               eventHandler.useCapture);
          JSEvents.eventHandlers.push(eventHandler);
        } else {
          for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
            if (JSEvents.eventHandlers[i].target == eventHandler.target
             && JSEvents.eventHandlers[i].eventTypeString == eventHandler.eventTypeString) {
               JSEvents._removeHandler(i--);
             }
          }
        }
        return 0;
      },
  removeSingleHandler(eventHandler) {
        let success = false;
        for (let i = 0; i < JSEvents.eventHandlers.length; ++i) {
          const handler = JSEvents.eventHandlers[i];
          if (handler.target === eventHandler.target
            && handler.eventTypeId === eventHandler.eventTypeId
            && handler.callbackfunc === eventHandler.callbackfunc
            && handler.userData === eventHandler.userData) {
            // in some very rare cases (ex: Safari / fullscreen events), there is more than 1 handler (eventTypeString is different)
            JSEvents._removeHandler(i--);
            success = true;
          }
        }
        return success ? 0 : -5;
      },
  getNodeNameForTarget(target) {
        if (!target) return '';
        if (target == window) return '#window';
        if (target == screen) return '#screen';
        return target?.nodeName || '';
      },
  fullscreenEnabled() {
        return document.fullscreenEnabled
        // Safari 13.0.3 on macOS Catalina 10.15.1 still ships with prefixed webkitFullscreenEnabled.
        // TODO: If Safari at some point ships with unprefixed version, update the version check above.
        || document.webkitFullscreenEnabled
         ;
      },
  };
  
  
  var stringToUTF8Array = (str, heap, outIdx, maxBytesToWrite) => {
      assert(typeof str === 'string', `stringToUTF8Array expects a string (got ${typeof str})`);
      // Parameter maxBytesToWrite is not optional. Negative values, 0, null,
      // undefined and false each don't write out any bytes.
      if (!(maxBytesToWrite > 0))
        return 0;
  
      var startIdx = outIdx;
      var endIdx = outIdx + maxBytesToWrite - 1; // -1 for string null terminator.
      for (var i = 0; i < str.length; ++i) {
        // For UTF8 byte structure, see http://en.wikipedia.org/wiki/UTF-8#Description
        // and https://www.ietf.org/rfc/rfc2279.txt
        // and https://tools.ietf.org/html/rfc3629
        var u = str.codePointAt(i);
        if (u <= 0x7F) {
          if (outIdx >= endIdx) break;
          heap[outIdx++] = u;
        } else if (u <= 0x7FF) {
          if (outIdx + 1 >= endIdx) break;
          heap[outIdx++] = 0xC0 | (u >> 6);
          heap[outIdx++] = 0x80 | (u & 63);
        } else if (u <= 0xFFFF) {
          if (outIdx + 2 >= endIdx) break;
          heap[outIdx++] = 0xE0 | (u >> 12);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
        } else {
          if (outIdx + 3 >= endIdx) break;
          if (u > 0x10FFFF) warnOnce(`Invalid Unicode code point ${ptrToString(u)} encountered when serializing a JS string to a UTF-8 string in wasm memory! (Valid unicode code points should be in range 0-0x10FFFF).`);
          heap[outIdx++] = 0xF0 | (u >> 18);
          heap[outIdx++] = 0x80 | ((u >> 12) & 63);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
          // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
          // We need to manually skip over the second code unit for correct iteration.
          i++;
        }
      }
      // Null-terminate the pointer to the buffer.
      heap[outIdx] = 0;
      return outIdx - startIdx;
    };
  var stringToUTF8 = (str, outPtr, maxBytesToWrite) => {
      assert(typeof maxBytesToWrite == 'number', 'stringToUTF8(str, outPtr, maxBytesToWrite) is missing the third parameter that specifies the length of the output buffer!');
      return stringToUTF8Array(str, HEAPU8, outPtr, maxBytesToWrite);
    };
  
  var registerKeyEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 160;
      JSEvents.keyEvent ||= _malloc(eventSize);
  
      var keyEventHandlerFunc = (e) => {
        assert(e);
  
        var keyEventData = JSEvents.keyEvent;
        HEAPF64[((keyEventData)>>3)] = e.timeStamp;
  
        var idx = ((keyEventData)>>2);
  
        HEAP32[idx + 2] = e.location;
        HEAP8[keyEventData + 12] = e.ctrlKey;
        HEAP8[keyEventData + 13] = e.shiftKey;
        HEAP8[keyEventData + 14] = e.altKey;
        HEAP8[keyEventData + 15] = e.metaKey;
        HEAP8[keyEventData + 16] = e.repeat;
        HEAP32[idx + 5] = e.charCode;
        HEAP32[idx + 6] = e.keyCode;
        HEAP32[idx + 7] = e.which;
        stringToUTF8(e.key || '', keyEventData + 32, 32);
        stringToUTF8(e.code || '', keyEventData + 64, 32);
        stringToUTF8(e.char || '', keyEventData + 96, 32);
        stringToUTF8(e.locale || '', keyEventData + 128, 32);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, keyEventData, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target: findEventTarget(target),
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: keyEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_keydown_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerKeyEventCallback(target, userData, useCapture, callbackfunc, 2, "keydown", targetThread);

  
  var fillMouseEventData = (eventStruct, e, target) => {
      assert(eventStruct % 4 == 0);
      HEAPF64[((eventStruct)>>3)] = e.timeStamp;
      var idx = ((eventStruct)>>2);
      HEAP32[idx + 2] = e.screenX;
      HEAP32[idx + 3] = e.screenY;
      HEAP32[idx + 4] = e.clientX;
      HEAP32[idx + 5] = e.clientY;
      HEAP8[eventStruct + 24] = e.ctrlKey;
      HEAP8[eventStruct + 25] = e.shiftKey;
      HEAP8[eventStruct + 26] = e.altKey;
      HEAP8[eventStruct + 27] = e.metaKey;
      HEAP16[idx*2 + 14] = e.button;
      HEAP16[idx*2 + 15] = e.buttons;
  
      HEAP32[idx + 8] = e["movementX"];
  
      HEAP32[idx + 9] = e["movementY"];
  
      // Note: rect contains doubles (truncated to placate SAFE_HEAP, which is the same behaviour when writing to HEAP32 anyway)
      var rect = getBoundingClientRect(target);
      HEAP32[idx + 10] = e.clientX - (rect.left | 0);
      HEAP32[idx + 11] = e.clientY - (rect.top  | 0);
    };
  
  
  var registerMouseEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 64;
      JSEvents.mouseEvent ||= _malloc(eventSize);
      target = findEventTarget(target);
  
      var mouseEventHandlerFunc = (e) => {
        // TODO: Make this access thread safe, or this could update live while app is reading it.
        fillMouseEventData(JSEvents.mouseEvent, e, target);
  
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, JSEvents.mouseEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: eventTypeString != 'mousemove' && eventTypeString != 'mouseenter' && eventTypeString != 'mouseleave', // Mouse move events do not allow fullscreen/pointer lock requests to be handled in them!
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: mouseEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_mousedown_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 5, "mousedown", targetThread);

  var _emscripten_set_mousemove_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 8, "mousemove", targetThread);

  var _emscripten_set_mouseup_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerMouseEventCallback(target, userData, useCapture, callbackfunc, 6, "mouseup", targetThread);

  
  
  var registerUiEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 36;
      JSEvents.uiEvent ||= _malloc(eventSize);
  
      target = findEventTarget(target);
  
      var uiEventHandlerFunc = (e) => {
        if (e.target != target) {
          // Never take ui events such as scroll via a 'bubbled' route, but always from the direct element that
          // was targeted. Otherwise e.g. if app logs a message in response to a page scroll, the Emscripten log
          // message box could cause to scroll, generating a new (bubbled) scroll message, causing a new log print,
          // causing a new scroll, etc..
          return;
        }
        var b = document.body; // Take document.body to a variable, Closure compiler does not outline access to it on its own.
        if (!b) {
          // During a page unload 'body' can be null, with "Cannot read property 'clientWidth' of null" being thrown
          return;
        }
        var uiEvent = JSEvents.uiEvent;
        HEAP32[((uiEvent)>>2)] = 0; // always zero for resize and scroll
        HEAP32[(((uiEvent)+(4))>>2)] = b.clientWidth;
        HEAP32[(((uiEvent)+(8))>>2)] = b.clientHeight;
        HEAP32[(((uiEvent)+(12))>>2)] = innerWidth;
        HEAP32[(((uiEvent)+(16))>>2)] = innerHeight;
        HEAP32[(((uiEvent)+(20))>>2)] = outerWidth;
        HEAP32[(((uiEvent)+(24))>>2)] = outerHeight;
        HEAP32[(((uiEvent)+(28))>>2)] = pageXOffset | 0; // scroll offsets are float
        HEAP32[(((uiEvent)+(32))>>2)] = pageYOffset | 0;
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, uiEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: uiEventHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  var _emscripten_set_resize_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) =>
      registerUiEventCallback(target, userData, useCapture, callbackfunc, 10, "resize", targetThread);

  
  
  var registerWheelEventCallback = (target, userData, useCapture, callbackfunc, eventTypeId, eventTypeString, targetThread) => {
      var eventSize = 96;
      JSEvents.wheelEvent ||= _malloc(eventSize)
  
      // The DOM Level 3 events spec event 'wheel'
      var wheelHandlerFunc = (e) => {
        var wheelEvent = JSEvents.wheelEvent;
        fillMouseEventData(wheelEvent, e, target);
        HEAPF64[(((wheelEvent)+(64))>>3)] = e["deltaX"];
        HEAPF64[(((wheelEvent)+(72))>>3)] = e["deltaY"];
        HEAPF64[(((wheelEvent)+(80))>>3)] = e["deltaZ"];
        HEAP32[(((wheelEvent)+(88))>>2)] = e["deltaMode"];
        if (((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId, wheelEvent, userData)) e.preventDefault();
      };
  
      var eventHandler = {
        target,
        allowsDeferredCalls: true,
        eventTypeString,
        eventTypeId,
        userData,
        callbackfunc,
        handlerFunc: wheelHandlerFunc,
        useCapture
      };
      return JSEvents.registerOrRemoveHandler(eventHandler);
    };
  
  var _emscripten_set_wheel_callback_on_thread = (target, userData, useCapture, callbackfunc, targetThread) => {
      target = findEventTarget(target);
      if (!target) return -4;
      if (typeof target.onwheel != 'undefined') {
        return registerWheelEventCallback(target, userData, useCapture, callbackfunc, 9, "wheel", targetThread);
      } else {
        return -1;
      }
    };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  var lengthBytesUTF8 = (str) => {
      var len = 0;
      for (var i = 0; i < str.length; ++i) {
        // Gotcha: charCodeAt returns a 16-bit word that is a UTF-16 encoded code
        // unit, not a Unicode code point of the character! So decode
        // UTF16->UTF32->UTF8.
        // See http://unicode.org/faq/utf_bom.html#utf16-3
        var c = str.charCodeAt(i); // possibly a lead surrogate
        if (c <= 0x7F) {
          len++;
        } else if (c <= 0x7FF) {
          len += 2;
        } else if (c >= 0xD800 && c <= 0xDFFF) {
          len += 4; ++i;
        } else {
          len += 3;
        }
      }
      return len;
    };
  
  
  var stackAlloc = (sz) => __emscripten_stack_alloc(sz);
  var stringToUTF8OnStack = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = stackAlloc(size);
      stringToUTF8(str, ret, size);
      return ret;
    };
  
  
  
  var readI53FromI64 = (ptr) => {
      return HEAPU32[((ptr)>>2)] + HEAP32[(((ptr)+(4))>>2)] * 4294967296;
    };
  
  var readI53FromU64 = (ptr) => {
      return HEAPU32[((ptr)>>2)] + HEAPU32[(((ptr)+(4))>>2)] * 4294967296;
    };
  var writeI53ToI64 = (ptr, num) => {
      HEAPU32[((ptr)>>2)] = num;
      var lower = HEAPU32[((ptr)>>2)];
      HEAPU32[(((ptr)+(4))>>2)] = (num - lower)/4294967296;
      var deserialized = (num >= 0) ? readI53FromU64(ptr) : readI53FromI64(ptr);
      var offset = ((ptr)>>2);
      if (deserialized != num) warnOnce(`writeI53ToI64() out of range: serialized JS Number ${num} to Wasm heap as bytes lo=${ptrToString(HEAPU32[offset])}, hi=${ptrToString(HEAPU32[offset+1])}, which deserializes back to ${deserialized} instead!`);
    };
  
  
  
  var stringToNewUTF8 = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = _malloc(size);
      if (ret) stringToUTF8(str, ret, size);
      return ret;
    };
  
  
  
  
  var WebGPU = {
  Internals:{
  jsObjects:[],
  jsObjectInsert:(ptr, jsObject) => {
          ptr >>>= 0
          WebGPU.Internals.jsObjects[ptr] = jsObject;
        },
  bufferOnUnmaps:[],
  futures:[],
  futureInsert:(futureId, promise) => {
          WebGPU.Internals.futures[futureId] =
            new Promise((resolve) => promise.finally(() => resolve(futureId)));
        },
  },
  getJsObject:(ptr) => {
        if (!ptr) return undefined;
        ptr >>>= 0
        assert(ptr in WebGPU.Internals.jsObjects);
        return WebGPU.Internals.jsObjects[ptr];
      },
  importJsAdapter:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateAdapter(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsBindGroup:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateBindGroup(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsBindGroupLayout:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateBindGroupLayout(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsBuffer:(buffer, parentPtr = 0) => {
        // At the moment, we do not allow importing pending buffers.
        assert(buffer.mapState === "unmapped");
        var bufferPtr = _emwgpuImportBuffer(parentPtr);
        WebGPU.Internals.jsObjectInsert(bufferPtr, buffer);
        return bufferPtr;
      },
  importJsCommandBuffer:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateCommandBuffer(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsCommandEncoder:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateCommandEncoder(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsComputePassEncoder:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateComputePassEncoder(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsComputePipeline:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateComputePipeline(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsDevice:(device, parentPtr = 0) => {
        var queuePtr = _emwgpuCreateQueue(parentPtr);
        var devicePtr = _emwgpuCreateDevice(parentPtr, queuePtr);
        WebGPU.Internals.jsObjectInsert(queuePtr, device.queue);
        WebGPU.Internals.jsObjectInsert(devicePtr, device);
        return devicePtr;
      },
  importJsExternalTexture:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateExternalTexture(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsPipelineLayout:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreatePipelineLayout(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsQuerySet:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateQuerySet(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsQueue:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateQueue(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsRenderBundle:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateRenderBundle(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsRenderBundleEncoder:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateRenderBundleEncoder(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsRenderPassEncoder:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateRenderPassEncoder(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsRenderPipeline:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateRenderPipeline(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsSampler:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateSampler(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsShaderModule:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateShaderModule(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsSurface:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateSurface(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsTexture:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateTexture(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  importJsTextureView:(obj, parentPtr = 0) => {
            var ptr = _emwgpuCreateTextureView(parentPtr);
            WebGPU.Internals.jsObjects[ptr] = obj;
            return ptr;
          },
  errorCallback:(callback, type, message, userdata) => {
        var sp = stackSave();
        var messagePtr = stringToUTF8OnStack(message);
        ((a1, a2, a3) => abort('Internal Error! Attempted to invoke wasm function pointer with signature "viii", but no such functions have gotten exported!'))(type, messagePtr, userdata);
        stackRestore(sp);
      },
  iterateExtensions:(root, handlers) => {
        assert(root);
        for (var ptr = HEAPU32[((root)>>2)]; ptr;
                 ptr = HEAPU32[((ptr)>>2)]) {
          var sType = HEAP32[(((ptr)+(4))>>2)];
          // This will crash if there's no handler indicating either a bogus
          // sType, or one we haven't implemented yet.
          var handler = handlers[sType](ptr);
        }
      },
  setStringView:(ptr, data, length) => {
        HEAPU32[((ptr)>>2)] = data;
        HEAPU32[(((ptr)+(4))>>2)] = length;
      },
  makeStringFromStringView:(stringViewPtr) => {
        var ptr = HEAPU32[((stringViewPtr)>>2)];
        var length = HEAPU32[(((stringViewPtr)+(4))>>2)];
        // UTF8ToString stops at the first null terminator character in the
        // string regardless of the length.
        return UTF8ToString(ptr, length);
      },
  makeStringFromOptionalStringView:(stringViewPtr) => {
        var ptr = HEAPU32[((stringViewPtr)>>2)];
        var length = HEAPU32[(((stringViewPtr)+(4))>>2)];
        // If we don't have a valid string pointer, just return undefined when
        // optional.
        if (!ptr) {
          if (length === 0) {
            return "";
          }
          return undefined;
        }
        // UTF8ToString stops at the first null terminator character in the
        // string regardless of the length.
        return UTF8ToString(ptr, length);
      },
  makeColor:(ptr) => {
        return {
          "r": HEAPF64[((ptr)>>3)],
          "g": HEAPF64[(((ptr)+(8))>>3)],
          "b": HEAPF64[(((ptr)+(16))>>3)],
          "a": HEAPF64[(((ptr)+(24))>>3)],
        };
      },
  makeExtent3D:(ptr) => {
        return {
          "width": HEAPU32[((ptr)>>2)],
          "height": HEAPU32[(((ptr)+(4))>>2)],
          "depthOrArrayLayers": HEAPU32[(((ptr)+(8))>>2)],
        };
      },
  makeOrigin3D:(ptr) => {
        return {
          "x": HEAPU32[((ptr)>>2)],
          "y": HEAPU32[(((ptr)+(4))>>2)],
          "z": HEAPU32[(((ptr)+(8))>>2)],
        };
      },
  makeTexelCopyTextureInfo:(ptr) => {
        assert(ptr);
        return {
          "texture": WebGPU.getJsObject(
            HEAPU32[((ptr)>>2)]),
          "mipLevel": HEAPU32[(((ptr)+(4))>>2)],
          "origin": WebGPU.makeOrigin3D(ptr + 8),
          "aspect": WebGPU.TextureAspect[HEAP32[(((ptr)+(20))>>2)]],
        };
      },
  makeTexelCopyBufferLayout:(ptr) => {
        var bytesPerRow = HEAPU32[(((ptr)+(8))>>2)];
        var rowsPerImage = HEAPU32[(((ptr)+(12))>>2)];
        return {
          "offset": readI53FromI64(ptr),
          "bytesPerRow": bytesPerRow === 4294967295 ? undefined : bytesPerRow,
          "rowsPerImage": rowsPerImage === 4294967295 ? undefined : rowsPerImage,
        };
      },
  makeTexelCopyBufferInfo:(ptr) => {
        assert(ptr);
        var layoutPtr = ptr + 0;
        var bufferCopyView = WebGPU.makeTexelCopyBufferLayout(layoutPtr);
        bufferCopyView["buffer"] = WebGPU.getJsObject(
          HEAPU32[(((ptr)+(16))>>2)]);
        return bufferCopyView;
      },
  makePassTimestampWrites:(ptr) => {
        if (ptr === 0) return undefined;
        return {
          "querySet": WebGPU.getJsObject(
            HEAPU32[(((ptr)+(4))>>2)]),
          "beginningOfPassWriteIndex": HEAPU32[(((ptr)+(8))>>2)],
          "endOfPassWriteIndex": HEAPU32[(((ptr)+(12))>>2)],
        };
      },
  makePipelineConstants:(constantCount, constantsPtr) => {
        if (!constantCount) return;
        var constants = {};
        for (var i = 0; i < constantCount; ++i) {
          var entryPtr = constantsPtr + 24 * i;
          var key = WebGPU.makeStringFromStringView(entryPtr + 4);
          constants[key] = HEAPF64[(((entryPtr)+(16))>>3)];
        }
        return constants;
      },
  makePipelineLayout:(layoutPtr) => {
        if (!layoutPtr) return 'auto';
        return WebGPU.getJsObject(layoutPtr);
      },
  makeComputeState:(ptr) => {
        if (!ptr) return undefined;
        assert(ptr);assert(HEAPU32[((ptr)>>2)] === 0);
        var desc = {
          "module": WebGPU.getJsObject(
            HEAPU32[(((ptr)+(4))>>2)]),
          "constants": WebGPU.makePipelineConstants(
            HEAPU32[(((ptr)+(16))>>2)],
            HEAPU32[(((ptr)+(20))>>2)]),
          "entryPoint": WebGPU.makeStringFromOptionalStringView(
            ptr + 8),
        };
        return desc;
      },
  makeComputePipelineDesc:(descriptor) => {
        assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
        var desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
          "layout": WebGPU.makePipelineLayout(
            HEAPU32[(((descriptor)+(12))>>2)]),
          "compute": WebGPU.makeComputeState(
            descriptor + 16),
        };
        return desc;
      },
  makeRenderPipelineDesc:(descriptor) => {
        assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
        function makePrimitiveState(psPtr) {
          if (!psPtr) return undefined;
          assert(psPtr);assert(HEAPU32[((psPtr)>>2)] === 0);
          return {
            "topology": WebGPU.PrimitiveTopology[HEAP32[(((psPtr)+(4))>>2)]],
            "stripIndexFormat": WebGPU.IndexFormat[HEAP32[(((psPtr)+(8))>>2)]],
            "frontFace": WebGPU.FrontFace[HEAP32[(((psPtr)+(12))>>2)]],
            "cullMode": WebGPU.CullMode[HEAP32[(((psPtr)+(16))>>2)]],
            "unclippedDepth": !!(HEAPU32[(((psPtr)+(20))>>2)]),
          };
        }
  
        function makeBlendComponent(bdPtr) {
          if (!bdPtr) return undefined;
          return {
            "operation": WebGPU.BlendOperation[HEAP32[((bdPtr)>>2)]],
            "srcFactor": WebGPU.BlendFactor[HEAP32[(((bdPtr)+(4))>>2)]],
            "dstFactor": WebGPU.BlendFactor[HEAP32[(((bdPtr)+(8))>>2)]],
          };
        }
  
        function makeBlendState(bsPtr) {
          if (!bsPtr) return undefined;
          return {
            "alpha": makeBlendComponent(bsPtr + 12),
            "color": makeBlendComponent(bsPtr + 0),
          };
        }
  
        function makeColorState(csPtr) {
          assert(csPtr);assert(HEAPU32[((csPtr)>>2)] === 0);
          var format = WebGPU.TextureFormat[HEAP32[(((csPtr)+(4))>>2)]];
          return format ? {
            "format": format,
            "blend": makeBlendState(HEAPU32[(((csPtr)+(8))>>2)]),
            "writeMask": HEAPU32[(((csPtr)+(16))>>2)],
          } : undefined;
        }
  
        function makeColorStates(count, csArrayPtr) {
          var states = [];
          for (var i = 0; i < count; ++i) {
            states.push(makeColorState(csArrayPtr + 24 * i));
          }
          return states;
        }
  
        function makeStencilStateFace(ssfPtr) {
          assert(ssfPtr);
          return {
            "compare": WebGPU.CompareFunction[HEAP32[((ssfPtr)>>2)]],
            "failOp": WebGPU.StencilOperation[HEAP32[(((ssfPtr)+(4))>>2)]],
            "depthFailOp": WebGPU.StencilOperation[HEAP32[(((ssfPtr)+(8))>>2)]],
            "passOp": WebGPU.StencilOperation[HEAP32[(((ssfPtr)+(12))>>2)]],
          };
        }
  
        function makeDepthStencilState(dssPtr) {
          if (!dssPtr) return undefined;
  
          assert(dssPtr);
          return {
            "format": WebGPU.TextureFormat[HEAP32[(((dssPtr)+(4))>>2)]],
            "depthWriteEnabled": !!(HEAPU32[(((dssPtr)+(8))>>2)]),
            "depthCompare": WebGPU.CompareFunction[HEAP32[(((dssPtr)+(12))>>2)]],
            "stencilFront": makeStencilStateFace(dssPtr + 16),
            "stencilBack": makeStencilStateFace(dssPtr + 32),
            "stencilReadMask": HEAPU32[(((dssPtr)+(48))>>2)],
            "stencilWriteMask": HEAPU32[(((dssPtr)+(52))>>2)],
            "depthBias": HEAP32[(((dssPtr)+(56))>>2)],
            "depthBiasSlopeScale": HEAPF32[(((dssPtr)+(60))>>2)],
            "depthBiasClamp": HEAPF32[(((dssPtr)+(64))>>2)],
          };
        }
  
        function makeVertexAttribute(vaPtr) {
          assert(vaPtr);
          return {
            "format": WebGPU.VertexFormat[HEAP32[(((vaPtr)+(4))>>2)]],
            "offset": readI53FromI64((vaPtr)+(8)),
            "shaderLocation": HEAPU32[(((vaPtr)+(16))>>2)],
          };
        }
  
        function makeVertexAttributes(count, vaArrayPtr) {
          var vas = [];
          for (var i = 0; i < count; ++i) {
            vas.push(makeVertexAttribute(vaArrayPtr + i * 24));
          }
          return vas;
        }
  
        function makeVertexBuffer(vbPtr) {
          if (!vbPtr) return undefined;
          var stepMode = WebGPU.VertexStepMode[HEAP32[(((vbPtr)+(4))>>2)]];
          var attributeCount = HEAPU32[(((vbPtr)+(16))>>2)];
          if (!stepMode && !attributeCount) {
            return null;
          }
          return {
            "arrayStride": readI53FromI64((vbPtr)+(8)),
            "stepMode": stepMode,
            "attributes": makeVertexAttributes(
              attributeCount,
              HEAPU32[(((vbPtr)+(20))>>2)]),
          };
        }
  
        function makeVertexBuffers(count, vbArrayPtr) {
          if (!count) return undefined;
  
          var vbs = [];
          for (var i = 0; i < count; ++i) {
            vbs.push(makeVertexBuffer(vbArrayPtr + i * 24));
          }
          return vbs;
        }
  
        function makeVertexState(viPtr) {
          if (!viPtr) return undefined;
          assert(viPtr);assert(HEAPU32[((viPtr)>>2)] === 0);
          var desc = {
            "module": WebGPU.getJsObject(
              HEAPU32[(((viPtr)+(4))>>2)]),
            "constants": WebGPU.makePipelineConstants(
              HEAPU32[(((viPtr)+(16))>>2)],
              HEAPU32[(((viPtr)+(20))>>2)]),
            "buffers": makeVertexBuffers(
              HEAPU32[(((viPtr)+(24))>>2)],
              HEAPU32[(((viPtr)+(28))>>2)]),
            "entryPoint": WebGPU.makeStringFromOptionalStringView(
              viPtr + 8),
            };
          return desc;
        }
  
        function makeMultisampleState(msPtr) {
          if (!msPtr) return undefined;
          assert(msPtr);assert(HEAPU32[((msPtr)>>2)] === 0);
          return {
            "count": HEAPU32[(((msPtr)+(4))>>2)],
            "mask": HEAPU32[(((msPtr)+(8))>>2)],
            "alphaToCoverageEnabled": !!(HEAPU32[(((msPtr)+(12))>>2)]),
          };
        }
  
        function makeFragmentState(fsPtr) {
          if (!fsPtr) return undefined;
          assert(fsPtr);assert(HEAPU32[((fsPtr)>>2)] === 0);
          var desc = {
            "module": WebGPU.getJsObject(
              HEAPU32[(((fsPtr)+(4))>>2)]),
            "constants": WebGPU.makePipelineConstants(
              HEAPU32[(((fsPtr)+(16))>>2)],
              HEAPU32[(((fsPtr)+(20))>>2)]),
            "targets": makeColorStates(
              HEAPU32[(((fsPtr)+(24))>>2)],
              HEAPU32[(((fsPtr)+(28))>>2)]),
            "entryPoint": WebGPU.makeStringFromOptionalStringView(
              fsPtr + 8),
            };
          return desc;
        }
  
        var desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
          "layout": WebGPU.makePipelineLayout(
            HEAPU32[(((descriptor)+(12))>>2)]),
          "vertex": makeVertexState(
            descriptor + 16),
          "primitive": makePrimitiveState(
            descriptor + 48),
          "depthStencil": makeDepthStencilState(
            HEAPU32[(((descriptor)+(72))>>2)]),
          "multisample": makeMultisampleState(
            descriptor + 76),
          "fragment": makeFragmentState(
            HEAPU32[(((descriptor)+(92))>>2)]),
        };
        return desc;
      },
  fillLimitStruct:(limits, limitsOutPtr) => {
        assert(limitsOutPtr);
        var nextInChainPtr = HEAPU32[((limitsOutPtr)>>2)];
  
        function setLimitValueU32(name, basePtr, limitOffset, fallbackValue = 0) {
          var limitValue = limits[name] ?? fallbackValue;
          HEAPU32[(((basePtr)+(limitOffset))>>2)] = limitValue;
        }
        function setLimitValueU64(name, basePtr, limitOffset, fallbackValue = 0) {
          var limitValue = limits[name] ?? fallbackValue;
          // Limits are integer-valued JS `Number`s, so they fit in 'i53'.
          writeI53ToI64((basePtr)+(limitOffset), limitValue);
        }
  
        setLimitValueU32('maxTextureDimension1D',                     limitsOutPtr, 4);
        setLimitValueU32('maxTextureDimension2D',                     limitsOutPtr, 8);
        setLimitValueU32('maxTextureDimension3D',                     limitsOutPtr, 12);
        setLimitValueU32('maxTextureArrayLayers',                     limitsOutPtr, 16);
        setLimitValueU32('maxBindGroups',                             limitsOutPtr, 20);
        setLimitValueU32('maxBindGroupsPlusVertexBuffers',            limitsOutPtr, 24);
        setLimitValueU32('maxBindingsPerBindGroup',                   limitsOutPtr, 28);
        setLimitValueU32('maxDynamicUniformBuffersPerPipelineLayout', limitsOutPtr, 32);
        setLimitValueU32('maxDynamicStorageBuffersPerPipelineLayout', limitsOutPtr, 36);
        setLimitValueU32('maxSampledTexturesPerShaderStage',          limitsOutPtr, 40);
        setLimitValueU32('maxSamplersPerShaderStage',                 limitsOutPtr, 44);
        setLimitValueU32('maxStorageBuffersPerShaderStage',           limitsOutPtr, 48);
        setLimitValueU32('maxStorageTexturesPerShaderStage',          limitsOutPtr, 52);
        setLimitValueU32('maxUniformBuffersPerShaderStage',           limitsOutPtr, 56);
        setLimitValueU32('minUniformBufferOffsetAlignment',           limitsOutPtr, 80);
        setLimitValueU32('minStorageBufferOffsetAlignment',           limitsOutPtr, 84);
        setLimitValueU64('maxUniformBufferBindingSize',               limitsOutPtr, 64);
        setLimitValueU64('maxStorageBufferBindingSize',               limitsOutPtr, 72);
        setLimitValueU32('maxVertexBuffers',                          limitsOutPtr, 88);
        setLimitValueU64('maxBufferSize',                             limitsOutPtr, 96);
        setLimitValueU32('maxVertexAttributes',                       limitsOutPtr, 104);
        setLimitValueU32('maxVertexBufferArrayStride',                limitsOutPtr, 108);
        setLimitValueU32('maxInterStageShaderVariables',              limitsOutPtr, 112);
        setLimitValueU32('maxColorAttachments',                       limitsOutPtr, 116);
        setLimitValueU32('maxColorAttachmentBytesPerSample',          limitsOutPtr, 120);
        setLimitValueU32('maxComputeWorkgroupStorageSize',            limitsOutPtr, 124);
        setLimitValueU32('maxComputeInvocationsPerWorkgroup',         limitsOutPtr, 128);
        setLimitValueU32('maxComputeWorkgroupSizeX',                  limitsOutPtr, 132);
        setLimitValueU32('maxComputeWorkgroupSizeY',                  limitsOutPtr, 136);
        setLimitValueU32('maxComputeWorkgroupSizeZ',                  limitsOutPtr, 140);
        setLimitValueU32('maxComputeWorkgroupsPerDimension',          limitsOutPtr, 144);
        // Note this limit is new and won't be present in all browsers for a while. Fall back to 0.
        setLimitValueU32('maxImmediateSize',                          limitsOutPtr, 148);
  
        if (nextInChainPtr !== 0) {
          var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
          assert(sType === 15);
          assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
          var compatibilityModeLimitsPtr = nextInChainPtr;
          assert(compatibilityModeLimitsPtr);assert(HEAPU32[((compatibilityModeLimitsPtr)>>2)] === 0);
  
          // Note these limits are new and won't be present in all browsers for a while. Fall back to exposing the PerShaderStage limit.
          setLimitValueU32('maxStorageBuffersInVertexStage',    compatibilityModeLimitsPtr, 8,    limits.maxStorageBuffersPerShaderStage);
          setLimitValueU32('maxStorageBuffersInFragmentStage',  compatibilityModeLimitsPtr, 16,  limits.maxStorageBuffersPerShaderStage);
          setLimitValueU32('maxStorageTexturesInVertexStage',   compatibilityModeLimitsPtr, 12,   limits.maxStorageTexturesPerShaderStage);
          setLimitValueU32('maxStorageTexturesInFragmentStage', compatibilityModeLimitsPtr, 20, limits.maxStorageTexturesPerShaderStage);
        }
      },
  fillAdapterInfoStruct:(info, infoStruct) => {
        assert(infoStruct);assert(HEAPU32[((infoStruct)>>2)] === 0);
  
        // Populate subgroup limits.
        HEAPU32[(((infoStruct)+(52))>>2)] = info.subgroupMinSize;
        HEAPU32[(((infoStruct)+(56))>>2)] = info.subgroupMaxSize;
  
        // Append all the strings together to condense into a single malloc.
        var strs = info.vendor + info.architecture + info.device + info.description;
        var strPtr = stringToNewUTF8(strs);
  
        var vendorLen = lengthBytesUTF8(info.vendor);
        WebGPU.setStringView(infoStruct + 4, strPtr, vendorLen);
        strPtr += vendorLen;
  
        var architectureLen = lengthBytesUTF8(info.architecture);
        WebGPU.setStringView(infoStruct + 12, strPtr, architectureLen);
        strPtr += architectureLen;
  
        var deviceLen = lengthBytesUTF8(info.device);
        WebGPU.setStringView(infoStruct + 20, strPtr, deviceLen);
        strPtr += deviceLen;
  
        var descriptionLen = lengthBytesUTF8(info.description);
        WebGPU.setStringView(infoStruct + 28, strPtr, descriptionLen);
        strPtr += descriptionLen;
  
        HEAP32[(((infoStruct)+(36))>>2)] = 2;
        var adapterType = info.isFallbackAdapter ? 3 : 4;
        HEAP32[(((infoStruct)+(40))>>2)] = adapterType;
        HEAPU32[(((infoStruct)+(44))>>2)] = 0;
        HEAPU32[(((infoStruct)+(48))>>2)] = 0;
      },
  AddressMode:[,"clamp-to-edge","repeat","mirror-repeat"],
  BlendFactor:[,"zero","one","src","one-minus-src","src-alpha","one-minus-src-alpha","dst","one-minus-dst","dst-alpha","one-minus-dst-alpha","src-alpha-saturated","constant","one-minus-constant","src1","one-minus-src1","src1-alpha","one-minus-src1-alpha"],
  BlendOperation:[,"add","subtract","reverse-subtract","min","max"],
  BufferBindingType:[,,"uniform","storage","read-only-storage"],
  BufferMapState:[,"unmapped","pending","mapped"],
  CompareFunction:[,"never","less","equal","less-equal","greater","not-equal","greater-equal","always"],
  CompilationInfoRequestStatus:[,"success","callback-cancelled"],
  ComponentSwizzle:[,"0","1","r","g","b","a"],
  CompositeAlphaMode:[,"opaque","premultiplied","unpremultiplied","inherit"],
  CullMode:[,"none","front","back"],
  ErrorFilter:[,"validation","out-of-memory","internal"],
  FeatureLevel:[,"compatibility","core"],
  FeatureName:{
  1:"core-features-and-limits",
  2:"depth-clip-control",
  3:"depth32float-stencil8",
  4:"texture-compression-bc",
  5:"texture-compression-bc-sliced-3d",
  6:"texture-compression-etc2",
  7:"texture-compression-astc",
  8:"texture-compression-astc-sliced-3d",
  9:"timestamp-query",
  10:"indirect-first-instance",
  11:"shader-f16",
  12:"rg11b10ufloat-renderable",
  13:"bgra8unorm-storage",
  14:"float32-filterable",
  15:"float32-blendable",
  16:"clip-distances",
  17:"dual-source-blending",
  18:"subgroups",
  19:"texture-formats-tier1",
  20:"texture-formats-tier2",
  21:"primitive-index",
  22:"texture-component-swizzle",
  327692:"chromium-experimental-unorm16-texture-formats",
  327729:"chromium-experimental-multi-draw-indirect",
  },
  FilterMode:[,"nearest","linear"],
  FrontFace:[,"ccw","cw"],
  IndexFormat:[,"uint16","uint32"],
  InstanceFeatureName:[,"timed-wait-any","shader-source-spirv","multiple-devices-per-adapter"],
  LoadOp:[,"load","clear"],
  MipmapFilterMode:[,"nearest","linear"],
  OptionalBool:["false","true",],
  PowerPreference:[,"low-power","high-performance"],
  PredefinedColorSpace:[,"srgb","display-p3"],
  PrimitiveTopology:[,"point-list","line-list","line-strip","triangle-list","triangle-strip"],
  QueryType:[,"occlusion","timestamp"],
  SamplerBindingType:[,,"filtering","non-filtering","comparison"],
  Status:[,"success","error"],
  StencilOperation:[,"keep","zero","replace","invert","increment-clamp","decrement-clamp","increment-wrap","decrement-wrap"],
  StorageTextureAccess:[,,"write-only","read-only","read-write"],
  StoreOp:[,"store","discard"],
  SurfaceGetCurrentTextureStatus:[,"success-optimal","success-suboptimal","timeout","outdated","lost","error"],
  TextureAspect:[,"all","stencil-only","depth-only"],
  TextureDimension:[,"1d","2d","3d"],
  TextureFormat:[,"r8unorm","r8snorm","r8uint","r8sint","r16unorm","r16snorm","r16uint","r16sint","r16float","rg8unorm","rg8snorm","rg8uint","rg8sint","r32float","r32uint","r32sint","rg16unorm","rg16snorm","rg16uint","rg16sint","rg16float","rgba8unorm","rgba8unorm-srgb","rgba8snorm","rgba8uint","rgba8sint","bgra8unorm","bgra8unorm-srgb","rgb10a2uint","rgb10a2unorm","rg11b10ufloat","rgb9e5ufloat","rg32float","rg32uint","rg32sint","rgba16unorm","rgba16snorm","rgba16uint","rgba16sint","rgba16float","rgba32float","rgba32uint","rgba32sint","stencil8","depth16unorm","depth24plus","depth24plus-stencil8","depth32float","depth32float-stencil8","bc1-rgba-unorm","bc1-rgba-unorm-srgb","bc2-rgba-unorm","bc2-rgba-unorm-srgb","bc3-rgba-unorm","bc3-rgba-unorm-srgb","bc4-r-unorm","bc4-r-snorm","bc5-rg-unorm","bc5-rg-snorm","bc6h-rgb-ufloat","bc6h-rgb-float","bc7-rgba-unorm","bc7-rgba-unorm-srgb","etc2-rgb8unorm","etc2-rgb8unorm-srgb","etc2-rgb8a1unorm","etc2-rgb8a1unorm-srgb","etc2-rgba8unorm","etc2-rgba8unorm-srgb","eac-r11unorm","eac-r11snorm","eac-rg11unorm","eac-rg11snorm","astc-4x4-unorm","astc-4x4-unorm-srgb","astc-5x4-unorm","astc-5x4-unorm-srgb","astc-5x5-unorm","astc-5x5-unorm-srgb","astc-6x5-unorm","astc-6x5-unorm-srgb","astc-6x6-unorm","astc-6x6-unorm-srgb","astc-8x5-unorm","astc-8x5-unorm-srgb","astc-8x6-unorm","astc-8x6-unorm-srgb","astc-8x8-unorm","astc-8x8-unorm-srgb","astc-10x5-unorm","astc-10x5-unorm-srgb","astc-10x6-unorm","astc-10x6-unorm-srgb","astc-10x8-unorm","astc-10x8-unorm-srgb","astc-10x10-unorm","astc-10x10-unorm-srgb","astc-12x10-unorm","astc-12x10-unorm-srgb","astc-12x12-unorm","astc-12x12-unorm-srgb"],
  TextureSampleType:[,,"float","unfilterable-float","depth","sint","uint"],
  TextureViewDimension:[,"1d","2d","2d-array","cube","cube-array","3d"],
  ToneMappingMode:[,"standard","extended"],
  VertexFormat:[,"uint8","uint8x2","uint8x4","sint8","sint8x2","sint8x4","unorm8","unorm8x2","unorm8x4","snorm8","snorm8x2","snorm8x4","uint16","uint16x2","uint16x4","sint16","sint16x2","sint16x4","unorm16","unorm16x2","unorm16x4","snorm16","snorm16x2","snorm16x4","float16","float16x2","float16x4","float32","float32x2","float32x3","float32x4","uint32","uint32x2","uint32x3","uint32x4","sint32","sint32x2","sint32x3","sint32x4","unorm10-10-10-2","unorm8x4-bgra"],
  VertexStepMode:[,"vertex","instance"],
  WGSLLanguageFeatureName:[,"readonly_and_readwrite_storage_textures","packed_4x8_integer_dot_product","unrestricted_pointer_parameters","pointer_composite_access","uniform_buffer_standard_layout","subgroup_id","texture_and_sampler_let","subgroup_uniformity","texture_formats_tier1","linear_indexing"],
  };
  
  var emwgpuStringToInt_DeviceLostReason = {
              'undefined': 1,  // For older browsers
              'unknown': 1,
              'destroyed': 2,
          };
  
  var handleException = (e) => {
      // Certain exception types we do not treat as errors since they are used for
      // internal control flow.
      // 1. ExitStatus, which is thrown by exit()
      // 2. "unwind", which is thrown by emscripten_unwind_to_js_event_loop() and others
      //    that wish to return to JS event loop.
      if (e instanceof ExitStatus || e == 'unwind') {
        return EXITSTATUS;
      }
      checkStackCookie();
      if (e instanceof WebAssembly.RuntimeError) {
        if (_emscripten_stack_get_current() <= 0) {
          err('Stack overflow detected.  You can try increasing -sSTACK_SIZE (currently set to 65536)');
        }
      }
      quit_(1, e);
    };
  
  
  var runtimeKeepaliveCounter = 0;
  var keepRuntimeAlive = () => noExitRuntime || runtimeKeepaliveCounter > 0;
  var _proc_exit = (code) => {
      EXITSTATUS = code;
      if (!keepRuntimeAlive()) {
        Module['onExit']?.(code);
        ABORT = true;
      }
      quit_(code, new ExitStatus(code));
    };
  
  
  /** @param {boolean|number=} implicit */
  var exitJS = (status, implicit) => {
      EXITSTATUS = status;
  
      checkUnflushedContent();
  
      // if exit() was called explicitly, warn the user if the runtime isn't actually being shut down
      if (keepRuntimeAlive() && !implicit) {
        var msg = `program exited (with status: ${status}), but keepRuntimeAlive() is set (counter=${runtimeKeepaliveCounter}) due to an async operation, so halting execution but not exiting the runtime or preventing further async execution (you can use emscripten_force_exit, if you want to force a true shutdown)`;
        err(msg);
      }
  
      _proc_exit(status);
    };
  var _exit = exitJS;
  
  
  var maybeExit = () => {
      if (!keepRuntimeAlive()) {
        try {
          _exit(EXITSTATUS);
        } catch (e) {
          handleException(e);
        }
      }
    };
  var callUserCallback = (func) => {
      if (ABORT) {
        err('user callback triggered after runtime exited or application aborted.  Ignoring.');
        return;
      }
      try {
        return func();
      } catch (e) {
        handleException(e);
      } finally {
        maybeExit();
      }
    };
  
  
  
  var INT53_MAX = 9007199254740992;
  
  var INT53_MIN = -9007199254740992;
  var bigintToI53Checked = (num) => (num < INT53_MIN || num > INT53_MAX) ? NaN : Number(num);
  function _emwgpuAdapterRequestDevice(adapterPtr, futureId, deviceLostFutureId, devicePtr, queuePtr, descriptor) {
    futureId = bigintToI53Checked(futureId);
    deviceLostFutureId = bigintToI53Checked(deviceLostFutureId);
  
  
      var adapter = WebGPU.getJsObject(adapterPtr);
  
      var desc = {};
      if (descriptor) {
        assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
        var requiredFeatureCount = HEAPU32[(((descriptor)+(12))>>2)];
        if (requiredFeatureCount) {
          var requiredFeaturesPtr = HEAPU32[(((descriptor)+(16))>>2)];
          // requiredFeaturesPtr is a pointer to an array of FeatureName which is an enum of size uint32_t
          desc["requiredFeatures"] = Array.from(HEAPU32.subarray((((requiredFeaturesPtr)>>2)), ((requiredFeaturesPtr + requiredFeatureCount * 4)>>2)),
            (feature) => WebGPU.FeatureName[feature]);
        }
        var limitsPtr = HEAPU32[(((descriptor)+(20))>>2)];
        if (limitsPtr) {
          assert(limitsPtr);
          var nextInChainPtr = HEAPU32[((limitsPtr)>>2)];
          var requiredLimits = {};
          function setLimitU32IfDefined(name, basePtr, limitOffset, ignoreIfZero = false) {
            var ptr = basePtr + limitOffset;
            var value = HEAPU32[((ptr)>>2)];
            if (value != 4294967295 && (!ignoreIfZero || value != 0)) {
              requiredLimits[name] = value;
            }
          }
          function setLimitU64IfDefined(name, basePtr, limitOffset) {
            var ptr = basePtr + limitOffset;
            // Handle WGPU_LIMIT_U64_UNDEFINED.
            var limitPart1 = HEAPU32[((ptr)>>2)];
            var limitPart2 = HEAPU32[(((ptr)+(4))>>2)];
            if (limitPart1 != 0xFFFFFFFF || limitPart2 != 0xFFFFFFFF) {
              requiredLimits[name] = readI53FromI64(ptr);
            }
          }
  
          setLimitU32IfDefined("maxTextureDimension1D",                     limitsPtr, 4);
          setLimitU32IfDefined("maxTextureDimension2D",                     limitsPtr, 8);
          setLimitU32IfDefined("maxTextureDimension3D",                     limitsPtr, 12);
          setLimitU32IfDefined("maxTextureArrayLayers",                     limitsPtr, 16);
          setLimitU32IfDefined("maxBindGroups",                             limitsPtr, 20);
          setLimitU32IfDefined('maxBindGroupsPlusVertexBuffers',            limitsPtr, 24);
          setLimitU32IfDefined('maxBindingsPerBindGroup',                   limitsPtr, 28);
          setLimitU32IfDefined("maxDynamicUniformBuffersPerPipelineLayout", limitsPtr, 32);
          setLimitU32IfDefined("maxDynamicStorageBuffersPerPipelineLayout", limitsPtr, 36);
          setLimitU32IfDefined("maxSampledTexturesPerShaderStage",          limitsPtr, 40);
          setLimitU32IfDefined("maxSamplersPerShaderStage",                 limitsPtr, 44);
          setLimitU32IfDefined("maxStorageBuffersPerShaderStage",           limitsPtr, 48);
          setLimitU32IfDefined("maxStorageTexturesPerShaderStage",          limitsPtr, 52);
          setLimitU32IfDefined("maxUniformBuffersPerShaderStage",           limitsPtr, 56);
          setLimitU32IfDefined("minUniformBufferOffsetAlignment",           limitsPtr, 80);
          setLimitU32IfDefined("minStorageBufferOffsetAlignment",           limitsPtr, 84);
          setLimitU64IfDefined("maxUniformBufferBindingSize",               limitsPtr, 64);
          setLimitU64IfDefined("maxStorageBufferBindingSize",               limitsPtr, 72);
          setLimitU32IfDefined("maxVertexBuffers",                          limitsPtr, 88);
          setLimitU64IfDefined("maxBufferSize",                             limitsPtr, 96);
          setLimitU32IfDefined("maxVertexAttributes",                       limitsPtr, 104);
          setLimitU32IfDefined("maxVertexBufferArrayStride",                limitsPtr, 108);
          setLimitU32IfDefined("maxInterStageShaderVariables",              limitsPtr, 112);
          setLimitU32IfDefined("maxColorAttachments",                       limitsPtr, 116);
          setLimitU32IfDefined("maxColorAttachmentBytesPerSample",          limitsPtr, 120);
          setLimitU32IfDefined("maxComputeWorkgroupStorageSize",            limitsPtr, 124);
          setLimitU32IfDefined("maxComputeInvocationsPerWorkgroup",         limitsPtr, 128);
          setLimitU32IfDefined("maxComputeWorkgroupSizeX",                  limitsPtr, 132);
          setLimitU32IfDefined("maxComputeWorkgroupSizeY",                  limitsPtr, 136);
          setLimitU32IfDefined("maxComputeWorkgroupSizeZ",                  limitsPtr, 140);
          setLimitU32IfDefined("maxComputeWorkgroupsPerDimension",          limitsPtr, 144);
          // Not present in all browsers. If the app requested 0, avoid passing it through so it won't cause an error.
          setLimitU32IfDefined("maxImmediateSize",                          limitsPtr, 148, true);
  
          if (nextInChainPtr !== 0) {
            var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
            assert(sType === 15);
            assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
            var compatibilityModeLimitsPtr = nextInChainPtr;
            assert(compatibilityModeLimitsPtr);assert(HEAPU32[((compatibilityModeLimitsPtr)>>2)] === 0);
            // If not present in the browser, don't request these, otherwise they'll cause an error.
            // (Technically, if any of these is higher than the PerShaderStage equivalent, we should
            // raise the PerShaderStage limit instead, but that's complex and apps should be able to
            // deal with that themselves.)
            if ('maxStorageBuffersInVertexStage' in GPUSupportedLimits.prototype) {
              setLimitU32IfDefined('maxStorageBuffersInVertexStage',    compatibilityModeLimitsPtr, 8);
              setLimitU32IfDefined('maxStorageTexturesInVertexStage',   compatibilityModeLimitsPtr, 12);
              setLimitU32IfDefined('maxStorageBuffersInFragmentStage',  compatibilityModeLimitsPtr, 16);
              setLimitU32IfDefined('maxStorageTexturesInFragmentStage', compatibilityModeLimitsPtr, 20);
            }
          }
  
          desc["requiredLimits"] = requiredLimits;
        }
  
        var defaultQueuePtr = HEAPU32[(((descriptor)+(24))>>2)];
        if (defaultQueuePtr) {
          var defaultQueueDesc = {
            "label": WebGPU.makeStringFromOptionalStringView(
              defaultQueuePtr + 4),
          };
          desc["defaultQueue"] = defaultQueueDesc;
        }
        desc["label"] = WebGPU.makeStringFromOptionalStringView(
          descriptor + 4
        );
      }
  
       // requestDevice
      WebGPU.Internals.futureInsert(futureId, adapter.requestDevice(desc).then((device) => {
         // requestDevice fulfilled
        callUserCallback(() => {
          WebGPU.Internals.jsObjectInsert(queuePtr, device.queue);
          WebGPU.Internals.jsObjectInsert(devicePtr, device);
  
          
  
          // Set up device lost promise resolution.
          assert(deviceLostFutureId);
          // Don't keepalive here, because this isn't guaranteed to ever happen.
          WebGPU.Internals.futureInsert(deviceLostFutureId, device.lost.then((info) => {
            // If the runtime has exited, avoid calling callUserCallback as it
            // will print an error (e.g. if the device got freed during shutdown).
            callUserCallback(() => {
              // Unset the uncaptured error handler.
              device.onuncapturederror = (ev) => {};
              var sp = stackSave();
              var messagePtr = stringToUTF8OnStack(info.message);
              _emwgpuOnDeviceLostCompleted(deviceLostFutureId, emwgpuStringToInt_DeviceLostReason[info.reason],
                messagePtr);
              stackRestore(sp);
            });
          }));
  
          // Set up uncaptured error handlers.
          assert(typeof GPUValidationError != 'undefined');
          assert(typeof GPUOutOfMemoryError != 'undefined');
          assert(typeof GPUInternalError != 'undefined');
          device.onuncapturederror = (ev) => {
              var type = 5;
              if (ev.error instanceof GPUValidationError) type = 2;
              else if (ev.error instanceof GPUOutOfMemoryError) type = 3;
              else if (ev.error instanceof GPUInternalError) type = 4;
              var sp = stackSave();
              var messagePtr = stringToUTF8OnStack(ev.error.message);
              _emwgpuOnUncapturedError(devicePtr, type, messagePtr);
              stackRestore(sp);
          };
  
          _emwgpuOnRequestDeviceCompleted(futureId, 1,
            devicePtr, 0);
        });
      }, (ex) => {
         // requestDevice rejected
        callUserCallback(() => {
          var sp = stackSave();
          var messagePtr = stringToUTF8OnStack(ex.message);
          _emwgpuOnRequestDeviceCompleted(futureId, 3,
            devicePtr, messagePtr);
          if (deviceLostFutureId) {
            _emwgpuOnDeviceLostCompleted(deviceLostFutureId, 4,
              messagePtr);
          }
          stackRestore(sp);
        });
      }));
    ;
  }

  
  
  
  
  var _emwgpuBufferGetConstMappedRange = (bufferPtr, offset, size) => {
      var buffer = WebGPU.getJsObject(bufferPtr);
  
      if (size === 0) warnOnce('getMappedRange size=0 no longer means WGPU_WHOLE_MAP_SIZE');
  
      if (size == -1) size = undefined;
  
      var mapped;
      try {
        mapped = buffer.getMappedRange(offset, size);
      } catch (ex) {
        err(`buffer.getMappedRange(${offset}, ${size}) failed: ${ex}`);
        return 0;
      }
      var data = _memalign(16, mapped.byteLength);
      HEAPU8.set(new Uint8Array(mapped), data);
      WebGPU.Internals.bufferOnUnmaps[bufferPtr].push(() => _free(data));
      return data;
    };

  
  
  
  
  var _emwgpuBufferMapAsync = function(bufferPtr, futureId, mode, offset, size) {
    futureId = bigintToI53Checked(futureId);
    mode = bigintToI53Checked(mode);
  
  
      var buffer = WebGPU.getJsObject(bufferPtr);
      WebGPU.Internals.bufferOnUnmaps[bufferPtr] = [];
  
      if (size == -1) size = undefined;
  
       // mapAsync
      WebGPU.Internals.futureInsert(futureId, buffer.mapAsync(mode, offset, size).then(() => {
         // mapAsync fulfilled
        callUserCallback(() => {
          _emwgpuOnMapAsyncCompleted(futureId, 1,
            0);
        });
      }, (ex) => {
         // mapAsync rejected
        callUserCallback(() => {
          var sp = stackSave();
          var messagePtr = stringToUTF8OnStack(ex.message);
          var status =
            ex.name === 'AbortError' ? 4 :
            ex.name === 'OperationError' ? 3 :
            0;
          assert(status);
          _emwgpuOnMapAsyncCompleted(futureId, status, messagePtr);
          delete WebGPU.Internals.bufferOnUnmaps[bufferPtr];
        });
      }));
    ;
  };

  
  var _emwgpuBufferUnmap = (bufferPtr) => {
      var buffer = WebGPU.getJsObject(bufferPtr);
  
      var onUnmap = WebGPU.Internals.bufferOnUnmaps[bufferPtr];
      if (!onUnmap) {
        // Already unmapped
        return;
      }
  
      for (var i = 0; i < onUnmap.length; ++i) {
        onUnmap[i]();
      }
      delete WebGPU.Internals.bufferOnUnmaps[bufferPtr]
  
      buffer.unmap();
    };

  
  var _emwgpuDelete = (ptr) => {
      delete WebGPU.Internals.jsObjects[ptr];
    };

  
  var _emwgpuDeviceCreateBuffer = (devicePtr, descriptor, bufferPtr) => {
      assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
      var mappedAtCreation = !!(HEAPU32[(((descriptor)+(32))>>2)]);
  
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "usage": HEAPU32[(((descriptor)+(16))>>2)],
        "size": readI53FromI64((descriptor)+(24)),
        "mappedAtCreation": mappedAtCreation,
      };
  
      var device = WebGPU.getJsObject(devicePtr);
      var buffer;
      try {
        buffer = device.createBuffer(desc);
      } catch (ex) {
        // The only exception should be RangeError if mapping at creation ran out of memory.
        assert(ex instanceof RangeError);
        assert(mappedAtCreation);
        err('createBuffer threw:', ex);
        return false;
      }
      WebGPU.Internals.jsObjectInsert(bufferPtr, buffer);
      if (mappedAtCreation) {
        WebGPU.Internals.bufferOnUnmaps[bufferPtr] = [];
      }
      return true;
    };

  
  var _emwgpuDeviceCreateShaderModule = (devicePtr, descriptor, shaderModulePtr) => {
      assert(descriptor);
      var nextInChainPtr = HEAPU32[((descriptor)>>2)];
      assert(nextInChainPtr !== 0);
      var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
  
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "code": "",
      };
  
      switch (sType) {
        case 2: {
          desc["code"] = WebGPU.makeStringFromStringView(
            nextInChainPtr + 8
          );
          break;
        }
        default: abort('unrecognized ShaderModule sType');
      }
  
      var device = WebGPU.getJsObject(devicePtr);
      WebGPU.Internals.jsObjectInsert(shaderModulePtr, device.createShaderModule(desc));
    };

  
  var _emwgpuDeviceDestroy = (devicePtr) => {
      const device = WebGPU.getJsObject(devicePtr);
      // Remove the onuncapturederror handler which holds a pointer to the WGPUDevice.
      device.onuncapturederror = null;
      device.destroy()
    };

  
  var emwgpuStringToInt_PreferredFormat = {
              'rgba8unorm': 22,
              'bgra8unorm': 27,
          };
  
  
  var _emwgpuGetPreferredFormat = () => {
      var format = navigator.gpu.getPreferredCanvasFormat();
      return emwgpuStringToInt_PreferredFormat[format];
    };

  
  
  
  
  function _emwgpuInstanceRequestAdapter(instancePtr, futureId, options, adapterPtr) {
    futureId = bigintToI53Checked(futureId);
  
  
      var opts;
      if (options) {
        assert(options);
        opts = {
          "featureLevel": WebGPU.FeatureLevel[HEAP32[(((options)+(4))>>2)]],
          "powerPreference": WebGPU.PowerPreference[HEAP32[(((options)+(8))>>2)]],
          "forceFallbackAdapter":
            !!(HEAPU32[(((options)+(12))>>2)]),
        };
  
        var nextInChainPtr = HEAPU32[((options)>>2)];
        if (nextInChainPtr !== 0) {
          var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
          assert(sType === 11);
          assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
          var webxrOptions = nextInChainPtr;
          assert(webxrOptions);assert(HEAPU32[((webxrOptions)>>2)] === 0);
          opts.xrCompatible = !!(HEAPU32[(((webxrOptions)+(8))>>2)]);
        }
      }
  
      if (!('gpu' in navigator)) {
        var sp = stackSave();
        var messagePtr = stringToUTF8OnStack('WebGPU not available on this browser (navigator.gpu is not available)');
        _emwgpuOnRequestAdapterCompleted(futureId, 3,
          adapterPtr, messagePtr);
        stackRestore(sp);
        return;
      }
  
       // requestAdapter
      WebGPU.Internals.futureInsert(futureId, navigator.gpu.requestAdapter(opts).then((adapter) => {
         // requestAdapter fulfilled
        callUserCallback(() => {
          if (adapter) {
            WebGPU.Internals.jsObjectInsert(adapterPtr, adapter);
            _emwgpuOnRequestAdapterCompleted(futureId, 1,
              adapterPtr, 0);
          } else {
            var sp = stackSave();
            var messagePtr = stringToUTF8OnStack('WebGPU not available on this browser (requestAdapter returned null)');
            _emwgpuOnRequestAdapterCompleted(futureId, 3,
              adapterPtr, messagePtr);
            stackRestore(sp);
          }
        });
      }, (ex) => {
         // requestAdapter rejected
        callUserCallback(() => {
          var sp = stackSave();
          var messagePtr = stringToUTF8OnStack(ex.message);
          _emwgpuOnRequestAdapterCompleted(futureId, 4,
            adapterPtr, messagePtr);
          stackRestore(sp);
        });
      }));
    ;
  }

  
  var _emwgpuWaitAny = (futurePtr, futureCount, timeoutMSPtr) => Asyncify.handleAsync(async () => {
      var promises = [];
      if (timeoutMSPtr) {
        var timeoutMS = HEAP32[((timeoutMSPtr)>>2)];
        promises.length = futureCount + 1;
        promises[futureCount] = new Promise((resolve) => setTimeout(resolve, timeoutMS, 0));
      } else {
        promises.length = futureCount;
      }
  
      for (var i = 0; i < futureCount; ++i) {
        // If any FutureID is not tracked, it means it must be done.
        var futureId = readI53FromI64((futurePtr + i * 8));
        if (!(futureId in WebGPU.Internals.futures)) {
          return futureId;
        }
        promises[i] = WebGPU.Internals.futures[futureId];
      }
  
      const firstResolvedFuture = await Promise.race(promises);
      delete WebGPU.Internals.futures[firstResolvedFuture];
      return firstResolvedFuture;
    });
  _emwgpuWaitAny.isAsync = true;

  var SYSCALLS = {
  varargs:undefined,
  getStr(ptr) {
        var ret = UTF8ToString(ptr);
        return ret;
      },
  };
  var _fd_close = (fd) => {
      abort('fd_close called without SYSCALLS_REQUIRE_FILESYSTEM');
    };

  function _fd_seek(fd, offset, whence, newOffset) {
    offset = bigintToI53Checked(offset);
  
  
      return 70;
    ;
  }

  var printCharBuffers = [null,[],[]];
  
  var printChar = (stream, curr) => {
      var buffer = printCharBuffers[stream];
      assert(buffer);
      if (curr === 0 || curr === 10) {
        (stream === 1 ? out : err)(UTF8ArrayToString(buffer));
        buffer.length = 0;
      } else {
        buffer.push(curr);
      }
    };
  
  var flush_NO_FILESYSTEM = () => {
      // flush anything remaining in the buffers during shutdown
      _fflush(0);
      if (printCharBuffers[1].length) printChar(1, 10);
      if (printCharBuffers[2].length) printChar(2, 10);
    };
  
  
  var _fd_write = (fd, iov, iovcnt, pnum) => {
      // hack to support printf in SYSCALLS_REQUIRE_FILESYSTEM=0
      var num = 0;
      for (var i = 0; i < iovcnt; i++) {
        var ptr = HEAPU32[((iov)>>2)];
        var len = HEAPU32[(((iov)+(4))>>2)];
        iov += 8;
        for (var j = 0; j < len; j++) {
          printChar(fd, HEAPU8[ptr+j]);
        }
        num += len;
      }
      HEAPU32[((pnum)>>2)] = num;
      return 0;
    };

  
  
  var _wgpuCommandEncoderBeginRenderPass = (encoderPtr, descriptor) => {
      assert(descriptor);
  
      function makeColorAttachment(caPtr) {
        var viewPtr = HEAPU32[(((caPtr)+(4))>>2)];
        if (viewPtr === 0) {
          // Null `view` means no attachment in this slot.
          return undefined;
        }
  
        var depthSlice = HEAPU32[(((caPtr)+(8))>>2)];
        if (depthSlice == 0xFFFFFFFF) depthSlice = undefined;
  
        return {
          "view": WebGPU.getJsObject(viewPtr),
          "depthSlice": depthSlice,
          "resolveTarget": WebGPU.getJsObject(
            HEAPU32[(((caPtr)+(12))>>2)]),
          "clearValue": WebGPU.makeColor(caPtr + 24),
          "loadOp": WebGPU.LoadOp[HEAP32[(((caPtr)+(16))>>2)]],
          "storeOp": WebGPU.StoreOp[HEAP32[(((caPtr)+(20))>>2)]],
        };
      }
  
      function makeColorAttachments(count, caPtr) {
        var attachments = [];
        for (var i = 0; i < count; ++i) {
          attachments.push(makeColorAttachment(caPtr + 56 * i));
        }
        return attachments;
      }
  
      function makeDepthStencilAttachment(dsaPtr) {
        if (dsaPtr === 0) return undefined;
  
        return {
          "view": WebGPU.getJsObject(
            HEAPU32[(((dsaPtr)+(4))>>2)]),
          "depthClearValue": HEAPF32[(((dsaPtr)+(16))>>2)],
          "depthLoadOp": WebGPU.LoadOp[HEAP32[(((dsaPtr)+(8))>>2)]],
          "depthStoreOp": WebGPU.StoreOp[HEAP32[(((dsaPtr)+(12))>>2)]],
          "depthReadOnly": !!(HEAPU32[(((dsaPtr)+(20))>>2)]),
          "stencilClearValue": HEAPU32[(((dsaPtr)+(32))>>2)],
          "stencilLoadOp": WebGPU.LoadOp[HEAP32[(((dsaPtr)+(24))>>2)]],
          "stencilStoreOp": WebGPU.StoreOp[HEAP32[(((dsaPtr)+(28))>>2)]],
          "stencilReadOnly": !!(HEAPU32[(((dsaPtr)+(36))>>2)]),
        };
      }
  
      function makeRenderPassDescriptor(descriptor) {
        assert(descriptor);
        var nextInChainPtr = HEAPU32[((descriptor)>>2)];
  
        var maxDrawCount = undefined;
        if (nextInChainPtr !== 0) {
          var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
          assert(sType === 3);
          assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
          var renderPassMaxDrawCount = nextInChainPtr;
          assert(renderPassMaxDrawCount);assert(HEAPU32[((renderPassMaxDrawCount)>>2)] === 0);
          // Note: The user could have passed a really huge value here, which is technically valid in
          // C but will not be allowed by WebGPU in JS because of [EnforceRange]. We intentionally
          // ignore that case because it's not useful - apps can just pick a smaller maxDrawCount.
          maxDrawCount = readI53FromI64((renderPassMaxDrawCount)+(8));
        }
  
        var desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
          "colorAttachments": makeColorAttachments(
            HEAPU32[(((descriptor)+(12))>>2)],
            HEAPU32[(((descriptor)+(16))>>2)]),
          "depthStencilAttachment": makeDepthStencilAttachment(
            HEAPU32[(((descriptor)+(20))>>2)]),
          "occlusionQuerySet": WebGPU.getJsObject(
            HEAPU32[(((descriptor)+(24))>>2)]),
          "timestampWrites": WebGPU.makePassTimestampWrites(
            HEAPU32[(((descriptor)+(28))>>2)]),
          "maxDrawCount": maxDrawCount,
        };
        return desc;
      }
  
      var desc = makeRenderPassDescriptor(descriptor);
  
      var commandEncoder = WebGPU.getJsObject(encoderPtr);
      var ptr = _emwgpuCreateRenderPassEncoder(0);
      WebGPU.Internals.jsObjectInsert(ptr, commandEncoder.beginRenderPass(desc));
      return ptr;
    };

  
  var _wgpuCommandEncoderCopyTextureToBuffer = (encoderPtr, srcPtr, dstPtr, copySizePtr) => {
      var commandEncoder = WebGPU.getJsObject(encoderPtr);
      var copySize = WebGPU.makeExtent3D(copySizePtr);
      commandEncoder.copyTextureToBuffer(
        WebGPU.makeTexelCopyTextureInfo(srcPtr), WebGPU.makeTexelCopyBufferInfo(dstPtr), copySize);
    };

  
  
  var _wgpuCommandEncoderFinish = (encoderPtr, descriptor) => {
      // TODO: Use the descriptor.
      var commandEncoder = WebGPU.getJsObject(encoderPtr);
      var ptr = _emwgpuCreateCommandBuffer(0);
      WebGPU.Internals.jsObjectInsert(ptr, commandEncoder.finish());
      return ptr;
    };

  
  
  var _wgpuDeviceCreateBindGroup = (devicePtr, descriptor) => {
      assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
      function makeEntry(entryPtr) {
        assert(entryPtr);
  
        var bufferPtr = HEAPU32[(((entryPtr)+(8))>>2)];
        var samplerPtr = HEAPU32[(((entryPtr)+(32))>>2)];
        var textureViewPtr = HEAPU32[(((entryPtr)+(36))>>2)];
        var externalTexturePtr = 0;
        WebGPU.iterateExtensions(entryPtr, {
          14: (ptr) => {
            externalTexturePtr = HEAPU32[(((ptr)+(8))>>2)];
          },
        });
        assert((bufferPtr !== 0) + (samplerPtr !== 0) + (textureViewPtr !== 0) + (externalTexturePtr !== 0) === 1);
  
        var resource;
        if (bufferPtr) {
          // Note the sentinel UINT64_MAX will be read as -1.
          var size = readI53FromI64((entryPtr)+(24));
          if (size == -1) size = undefined;
  
          resource = {
            "buffer": WebGPU.getJsObject(bufferPtr),
            "offset": readI53FromI64((entryPtr)+(16)),
            "size": size,
          };
        } else {
          resource = WebGPU.getJsObject(samplerPtr || textureViewPtr || externalTexturePtr);
        }
        return {
          "binding": HEAPU32[(((entryPtr)+(4))>>2)],
          "resource": resource,
        };
      }
  
      function makeEntries(count, entriesPtrs) {
        var entries = [];
        for (var i = 0; i < count; ++i) {
          entries.push(makeEntry(entriesPtrs +
              40 * i));
        }
        return entries;
      }
  
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "layout": WebGPU.getJsObject(
          HEAPU32[(((descriptor)+(12))>>2)]),
        "entries": makeEntries(
          HEAPU32[(((descriptor)+(16))>>2)],
          HEAPU32[(((descriptor)+(20))>>2)]
        ),
      };
  
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateBindGroup(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createBindGroup(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreateBindGroupLayout = (devicePtr, descriptor) => {
      assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
      function makeBufferEntry(substructPtr) {
        var typeInt =
          HEAPU32[(((substructPtr)+(4))>>2)];
        if (!typeInt) return undefined;
  
        return {
          "type": WebGPU.BufferBindingType[typeInt],
          "hasDynamicOffset":
            !!(HEAPU32[(((substructPtr)+(8))>>2)]),
          "minBindingSize":
            readI53FromI64((substructPtr)+(16)),
        };
      }
  
      function makeSamplerEntry(substructPtr) {
        var typeInt =
          HEAPU32[(((substructPtr)+(4))>>2)];
        if (!typeInt) return undefined;
  
        return {
          "type": WebGPU.SamplerBindingType[typeInt],
        };
      }
  
      function makeTextureEntry(substructPtr) {
        var sampleTypeInt =
          HEAPU32[(((substructPtr)+(4))>>2)];
        if (!sampleTypeInt) return undefined;
  
        return {
          "sampleType": WebGPU.TextureSampleType[sampleTypeInt],
          "viewDimension": WebGPU.TextureViewDimension[HEAP32[(((substructPtr)+(8))>>2)]],
          "multisampled":
            !!(HEAPU32[(((substructPtr)+(12))>>2)]),
        };
      }
  
      function makeStorageTextureEntry(substructPtr) {
        var accessInt =
          HEAPU32[(((substructPtr)+(4))>>2)]
        if (!accessInt) return undefined;
  
        return {
          "access": WebGPU.StorageTextureAccess[accessInt],
          "format": WebGPU.TextureFormat[HEAP32[(((substructPtr)+(8))>>2)]],
          "viewDimension": WebGPU.TextureViewDimension[HEAP32[(((substructPtr)+(12))>>2)]],
        };
      }
  
      function makeEntry(entryPtr) {
        assert(entryPtr);
        // bindingArraySize is not specced and thus not implemented yet. We don't pass it through
        // because if we did, then existing apps using this version of the bindings could break when
        // browsers start accepting bindingArraySize.
        var bindingArraySize = HEAPU32[(((entryPtr)+(16))>>2)];
        assert(bindingArraySize == 0 || bindingArraySize == 1);
  
        var entry = {
          "binding":
            HEAPU32[(((entryPtr)+(4))>>2)],
          "visibility":
            HEAPU32[(((entryPtr)+(8))>>2)],
          "buffer": makeBufferEntry(entryPtr + 24),
          "sampler": makeSamplerEntry(entryPtr + 48),
          "texture": makeTextureEntry(entryPtr + 56),
          "storageTexture": makeStorageTextureEntry(entryPtr + 72),
        };
        WebGPU.iterateExtensions(entryPtr, {
          13: (ptr) => {
            entry["externalTexture"] = {};
          },
        });
        return entry;
      }
  
      function makeEntries(count, entriesPtrs) {
        var entries = [];
        for (var i = 0; i < count; ++i) {
          entries.push(makeEntry(entriesPtrs +
              88 * i));
        }
        return entries;
      }
  
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "entries": makeEntries(
          HEAPU32[(((descriptor)+(12))>>2)],
          HEAPU32[(((descriptor)+(16))>>2)]
        ),
      };
  
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateBindGroupLayout(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createBindGroupLayout(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreateCommandEncoder = (devicePtr, descriptor) => {
      var desc;
      if (descriptor) {
        assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
        desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
        };
      }
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateCommandEncoder(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createCommandEncoder(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreatePipelineLayout = (devicePtr, descriptor) => {
      assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
      var bglCount = HEAPU32[(((descriptor)+(12))>>2)];
      var bglPtr = HEAPU32[(((descriptor)+(16))>>2)];
      var bgls = [];
      for (var i = 0; i < bglCount; ++i) {
        bgls.push(WebGPU.getJsObject(
          HEAPU32[(((bglPtr)+(4 * i))>>2)]));
      }
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "bindGroupLayouts": bgls,
        "immediateSize": HEAPU32[(((descriptor)+(20))>>2)],
      };
  
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreatePipelineLayout(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createPipelineLayout(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreateRenderPipeline = (devicePtr, descriptor) => {
      var desc = WebGPU.makeRenderPipelineDesc(descriptor);
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateRenderPipeline(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createRenderPipeline(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreateSampler = (devicePtr, descriptor) => {
      var desc;
      if (descriptor) {
        assert(descriptor);assert(HEAPU32[((descriptor)>>2)] === 0);
  
        desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
          "addressModeU": WebGPU.AddressMode[HEAP32[(((descriptor)+(12))>>2)]],
          "addressModeV": WebGPU.AddressMode[HEAP32[(((descriptor)+(16))>>2)]],
          "addressModeW": WebGPU.AddressMode[HEAP32[(((descriptor)+(20))>>2)]],
          "magFilter": WebGPU.FilterMode[HEAP32[(((descriptor)+(24))>>2)]],
          "minFilter": WebGPU.FilterMode[HEAP32[(((descriptor)+(28))>>2)]],
          "mipmapFilter": WebGPU.MipmapFilterMode[HEAP32[(((descriptor)+(32))>>2)]],
          "lodMinClamp": HEAPF32[(((descriptor)+(36))>>2)],
          "lodMaxClamp": HEAPF32[(((descriptor)+(40))>>2)],
          "compare": WebGPU.CompareFunction[HEAP32[(((descriptor)+(44))>>2)]],
          "maxAnisotropy": HEAPU16[(((descriptor)+(48))>>1)],
        };
      }
  
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateSampler(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createSampler(desc));
      return ptr;
    };

  
  
  var _wgpuDeviceCreateTexture = (devicePtr, descriptor) => {
      assert(descriptor);
      var nextInChainPtr = HEAPU32[((descriptor)>>2)];
  
      var textureBindingViewDimension;
      if (nextInChainPtr !== 0) {
        var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
        assert(sType === 16);
        assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
        var textureBindingViewDimensionDescriptor = nextInChainPtr;
        assert(textureBindingViewDimensionDescriptor);assert(HEAPU32[((textureBindingViewDimensionDescriptor)>>2)] === 0);
        textureBindingViewDimension = WebGPU.TextureViewDimension[HEAP32[(((textureBindingViewDimensionDescriptor)+(8))>>2)]];
      }
  
      var desc = {
        "label": WebGPU.makeStringFromOptionalStringView(
          descriptor + 4),
        "size": WebGPU.makeExtent3D(descriptor + 28),
        "mipLevelCount": HEAPU32[(((descriptor)+(44))>>2)],
        "sampleCount": HEAPU32[(((descriptor)+(48))>>2)],
        "dimension": WebGPU.TextureDimension[HEAP32[(((descriptor)+(24))>>2)]],
        "format": WebGPU.TextureFormat[HEAP32[(((descriptor)+(40))>>2)]],
        "usage": HEAPU32[(((descriptor)+(16))>>2)],
        "textureBindingViewDimension": textureBindingViewDimension,
      };
  
      var viewFormatCount = HEAPU32[(((descriptor)+(52))>>2)];
      if (viewFormatCount) {
        var viewFormatsPtr = HEAPU32[(((descriptor)+(56))>>2)];
        // viewFormatsPtr pointer to an array of TextureFormat which is an enum of size uint32_t
        desc['viewFormats'] = Array.from(HEAP32.subarray((((viewFormatsPtr)>>2)), ((viewFormatsPtr + viewFormatCount * 4)>>2)),
          format => WebGPU.TextureFormat[format]);
      }
  
      var device = WebGPU.getJsObject(devicePtr);
      var ptr = _emwgpuCreateTexture(0);
      WebGPU.Internals.jsObjectInsert(ptr, device.createTexture(desc));
      return ptr;
    };

  
  
  
  var _wgpuInstanceCreateSurface = (instancePtr, descriptor) => {
      assert(descriptor);
      var nextInChainPtr = HEAPU32[((descriptor)>>2)];
      assert(nextInChainPtr !== 0);
      assert(262144 ===
        HEAP32[(((nextInChainPtr)+(4))>>2)]);
      var sourceCanvasHTMLSelector = nextInChainPtr;
  
      assert(sourceCanvasHTMLSelector);assert(HEAPU32[((sourceCanvasHTMLSelector)>>2)] === 0);
      var selectorPtr = HEAPU32[(((sourceCanvasHTMLSelector)+(8))>>2)];
      assert(selectorPtr);
      var canvas = findCanvasEventTarget(selectorPtr);
      var context = canvas.getContext('webgpu');
      assert(context);
      if (!context) return 0;
  
      context.surfaceLabelWebGPU = WebGPU.makeStringFromOptionalStringView(
        descriptor + 4
      );
  
      var ptr = _emwgpuCreateSurface(0);
      WebGPU.Internals.jsObjectInsert(ptr, context);
      return ptr;
    };

  
  var _wgpuQueueSubmit = (queuePtr, commandCount, commands) => {
      assert(commands % 4 === 0);
      var queue = WebGPU.getJsObject(queuePtr);
      var cmds = Array.from(HEAP32.subarray((((commands)>>2)), ((commands + commandCount * 4)>>2)),
        (id) => WebGPU.getJsObject(id));
      queue.submit(cmds);
    };

  
  
  function _wgpuQueueWriteBuffer(queuePtr, bufferPtr, bufferOffset, data, size) {
    bufferOffset = bigintToI53Checked(bufferOffset);
  
  
      var queue = WebGPU.getJsObject(queuePtr);
      var buffer = WebGPU.getJsObject(bufferPtr);
      // There is a size limitation for ArrayBufferView. Work around by passing in a subarray
      // instead of the whole heap. crbug.com/1201109
      var subarray = HEAPU8.subarray(data, data + size);
      queue.writeBuffer(buffer, bufferOffset, subarray, 0, size);
    ;
  }

  
  var _wgpuQueueWriteTexture = (queuePtr, destinationPtr, data, dataSize, dataLayoutPtr, writeSizePtr) => {
      var queue = WebGPU.getJsObject(queuePtr);
  
      var destination = WebGPU.makeTexelCopyTextureInfo(destinationPtr);
      var dataLayout = WebGPU.makeTexelCopyBufferLayout(dataLayoutPtr);
      var writeSize = WebGPU.makeExtent3D(writeSizePtr);
      // This subarray isn't strictly necessary, but helps work around an issue
      // where Chromium makes a copy of the entire heap. crbug.com/1134457
      var subarray = HEAPU8.subarray(data, data + dataSize);
      queue.writeTexture(destination, subarray, dataLayout, writeSize);
    };

  
  var _wgpuRenderPassEncoderDraw = (passPtr, vertexCount, instanceCount, firstVertex, firstInstance) => {
      assert(vertexCount >= 0);
      assert(instanceCount >= 0);
      firstVertex >>>= 0;
      firstInstance >>>= 0;
      var pass = WebGPU.getJsObject(passPtr);
      pass.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    };

  
  var _wgpuRenderPassEncoderEnd = (encoderPtr) => {
      var encoder = WebGPU.getJsObject(encoderPtr);
      encoder.end();
    };

  
  var _wgpuRenderPassEncoderSetBindGroup = (passPtr, groupIndex, groupPtr, dynamicOffsetCount, dynamicOffsetsPtr) => {
      assert(groupIndex >= 0);
      var pass = WebGPU.getJsObject(passPtr);
      var group = WebGPU.getJsObject(groupPtr);
      if (dynamicOffsetCount == 0) {
        pass.setBindGroup(groupIndex, group);
      } else {
        pass.setBindGroup(groupIndex, group, HEAPU32, ((dynamicOffsetsPtr)>>2), dynamicOffsetCount);
      }
    };

  
  var _wgpuRenderPassEncoderSetPipeline = (passPtr, pipelinePtr) => {
      var pass = WebGPU.getJsObject(passPtr);
      var pipeline = WebGPU.getJsObject(pipelinePtr);
      pass.setPipeline(pipeline);
    };

  
  
  function _wgpuRenderPassEncoderSetVertexBuffer(passPtr, slot, bufferPtr, offset, size) {
    offset = bigintToI53Checked(offset);
    size = bigintToI53Checked(size);
  
  
      assert(slot >= 0);
      var pass = WebGPU.getJsObject(passPtr);
      var buffer = WebGPU.getJsObject(bufferPtr);
      if (size == -1) size = undefined;
      pass.setVertexBuffer(slot, buffer, offset, size);
    ;
  }

  
  var _wgpuSurfaceConfigure = (surfacePtr, config) => {
      assert(config);
      var context = WebGPU.getJsObject(surfacePtr);
  
      var presentMode = HEAPU32[(((config)+(44))>>2)];
      assert(presentMode === 1 ||
             presentMode === 0);
  
      var canvasSize = [
        HEAPU32[(((config)+(24))>>2)],
        HEAPU32[(((config)+(28))>>2)]
      ];
  
      if (canvasSize[0] !== 0) {
        context["canvas"]["width"] = canvasSize[0];
      }
  
      if (canvasSize[1] !== 0) {
        context["canvas"]["height"] = canvasSize[1];
      }
  
      var configuration = {
        "device": WebGPU.getJsObject(HEAPU32[(((config)+(4))>>2)]),
        "format": WebGPU.TextureFormat[HEAP32[(((config)+(8))>>2)]],
        "usage": HEAPU32[(((config)+(16))>>2)],
        "alphaMode": WebGPU.CompositeAlphaMode[HEAP32[(((config)+(40))>>2)]],
      };
  
      var viewFormatCount = HEAPU32[(((config)+(32))>>2)];
      if (viewFormatCount) {
        var viewFormatsPtr = HEAPU32[(((config)+(36))>>2)];
        // viewFormatsPtr pointer to an array of TextureFormat which is an enum of size uint32_t
        configuration['viewFormats'] = Array.from(HEAP32.subarray((((viewFormatsPtr)>>2)), ((viewFormatsPtr + viewFormatCount * 4)>>2)),
          format => WebGPU.TextureFormat[format]);
      }
  
      {
        var nextInChainPtr = HEAPU32[((config)>>2)];
  
        if (nextInChainPtr !== 0) {
          var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
          assert(sType === 10);
          assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
          var surfaceColorManagement = nextInChainPtr;
          assert(surfaceColorManagement);assert(HEAPU32[((surfaceColorManagement)>>2)] === 0);
          configuration.colorSpace = WebGPU.PredefinedColorSpace[HEAP32[(((surfaceColorManagement)+(8))>>2)]];
          configuration.toneMapping = {
            mode: WebGPU.ToneMappingMode[HEAP32[(((surfaceColorManagement)+(12))>>2)]],
          };
        }
      }
  
      context.configure(configuration);
    };

  
  
  var _wgpuSurfaceGetCurrentTexture = (surfacePtr, surfaceTexturePtr) => {
      assert(surfaceTexturePtr);
      var context = WebGPU.getJsObject(surfacePtr);
  
      try {
        var texturePtr = _emwgpuCreateTexture(0);
        WebGPU.Internals.jsObjectInsert(texturePtr, context.getCurrentTexture());
        HEAPU32[(((surfaceTexturePtr)+(4))>>2)] = texturePtr;
        HEAP32[(((surfaceTexturePtr)+(8))>>2)] = 1;
      } catch (ex) {
        err(`wgpuSurfaceGetCurrentTexture() failed: ${ex}`);
        HEAPU32[(((surfaceTexturePtr)+(4))>>2)] = 0;
        HEAP32[(((surfaceTexturePtr)+(8))>>2)] = 6;
      }
    };

  
  
  var _wgpuTextureCreateView = (texturePtr, descriptor) => {
      var desc;
      if (descriptor) {
        var swizzle;
        var nextInChainPtr = HEAPU32[((descriptor)>>2)];
        if (nextInChainPtr !== 0) {
          var sType = HEAP32[(((nextInChainPtr)+(4))>>2)];
          assert(sType === 12);
          assert(0 === HEAPU32[((nextInChainPtr)>>2)]);
          var swizzleDescriptor = nextInChainPtr;
          assert(swizzleDescriptor);assert(HEAPU32[((swizzleDescriptor)>>2)] === 0);
          var swizzlePtr = swizzleDescriptor + 8;
          var r = WebGPU.ComponentSwizzle[HEAP32[((swizzlePtr)>>2)]] || 'r';
          var g = WebGPU.ComponentSwizzle[HEAP32[(((swizzlePtr)+(4))>>2)]] || 'g';
          var b = WebGPU.ComponentSwizzle[HEAP32[(((swizzlePtr)+(8))>>2)]] || 'b';
          var a = WebGPU.ComponentSwizzle[HEAP32[(((swizzlePtr)+(12))>>2)]] || 'a';
          swizzle = `${r}${g}${b}${a}`;
        }
  
        var mipLevelCount = HEAPU32[(((descriptor)+(24))>>2)];
        var arrayLayerCount = HEAPU32[(((descriptor)+(32))>>2)];
        desc = {
          "label": WebGPU.makeStringFromOptionalStringView(
            descriptor + 4),
          "format": WebGPU.TextureFormat[HEAP32[(((descriptor)+(12))>>2)]],
          "dimension": WebGPU.TextureViewDimension[HEAP32[(((descriptor)+(16))>>2)]],
          "baseMipLevel": HEAPU32[(((descriptor)+(20))>>2)],
          "mipLevelCount": mipLevelCount === 4294967295 ? undefined : mipLevelCount,
          "baseArrayLayer": HEAPU32[(((descriptor)+(28))>>2)],
          "arrayLayerCount": arrayLayerCount === 4294967295 ? undefined : arrayLayerCount,
          "aspect": WebGPU.TextureAspect[HEAP32[(((descriptor)+(36))>>2)]],
          "usage": HEAPU32[(((descriptor)+(40))>>2)],
          "swizzle": swizzle,
        };
      }
  
      var texture = WebGPU.getJsObject(texturePtr);
      var ptr = _emwgpuCreateTextureView(0);
      WebGPU.Internals.jsObjectInsert(ptr, texture.createView(desc));
      return ptr;
    };



  var runAndAbortIfError = (func) => {
      try {
        return func();
      } catch (e) {
        abort(e);
      }
    };
  
  
  var createNamedFunction = (name, func) => Object.defineProperty(func, 'name', { value: name });
  
  var runtimeKeepalivePush = () => {
      runtimeKeepaliveCounter += 1;
    };
  
  var runtimeKeepalivePop = () => {
      assert(runtimeKeepaliveCounter > 0);
      runtimeKeepaliveCounter -= 1;
    };
  
  
  var Asyncify = {
  instrumentWasmImports(imports) {
        var importPattern = /^(invoke_.*|__asyncjs__.*)$/;
  
        for (let [x, original] of Object.entries(imports)) {
          if (typeof original == 'function') {
            let isAsyncifyImport = original.isAsync || importPattern.test(x);
            imports[x] = (...args) => {
              var originalAsyncifyState = Asyncify.state;
              try {
                return original(...args);
              } finally {
                // Only asyncify-declared imports are allowed to change the
                // state.
                // Changing the state from normal to disabled is allowed (in any
                // function) as that is what shutdown does (and we don't have an
                // explicit list of shutdown imports).
                var changedToDisabled =
                      originalAsyncifyState === Asyncify.State.Normal &&
                      Asyncify.state        === Asyncify.State.Disabled;
                // invoke_* functions are allowed to change the state if we do
                // not ignore indirect calls.
                var ignoredInvoke = x.startsWith('invoke_') &&
                                    true;
                if (Asyncify.state !== originalAsyncifyState &&
                    !isAsyncifyImport &&
                    !changedToDisabled &&
                    !ignoredInvoke) {
                  abort(`import ${x} was not in ASYNCIFY_IMPORTS, but changed the state`);
                }
              }
            };
          }
        }
      },
  instrumentFunction(original) {
        var wrapper = (...args) => {
          Asyncify.exportCallStack.push(original);
          try {
            return original(...args);
          } finally {
            if (!ABORT) {
              var top = Asyncify.exportCallStack.pop();
              assert(top === original);
              Asyncify.maybeStopUnwind();
            }
          }
        };
        Asyncify.funcWrappers.set(original, wrapper);
        wrapper = createNamedFunction(`__asyncify_wrapper_${original.name}`, wrapper);
        return wrapper;
      },
  instrumentWasmExports(exports) {
        var ret = {};
        for (let [x, original] of Object.entries(exports)) {
          if (typeof original == 'function') {
            var wrapper = Asyncify.instrumentFunction(original);
            ret[x] = wrapper;
          } else {
            ret[x] = original;
          }
        }
        return ret;
      },
  State:{
  Normal:0,
  Unwinding:1,
  Rewinding:2,
  Disabled:3,
  },
  state:0,
  StackSize:4096,
  currData:null,
  handleSleepReturnValue:0,
  exportCallStack:[],
  callstackFuncToId:new Map,
  callStackIdToFunc:new Map,
  funcWrappers:new Map,
  callStackId:0,
  asyncPromiseHandlers:null,
  sleepCallbacks:[],
  getCallStackId(func) {
        assert(func);
        if (!Asyncify.callstackFuncToId.has(func)) {
          var id = Asyncify.callStackId++;
          Asyncify.callstackFuncToId.set(func, id);
          Asyncify.callStackIdToFunc.set(id, func);
        }
        return Asyncify.callstackFuncToId.get(func);
      },
  maybeStopUnwind() {
        if (Asyncify.currData &&
            Asyncify.state === Asyncify.State.Unwinding &&
            Asyncify.exportCallStack.length === 0) {
          // We just finished unwinding.
          // Be sure to set the state before calling any other functions to avoid
          // possible infinite recursion here (For example in debug pthread builds
          // the dbg() function itself can call back into WebAssembly to get the
          // current pthread_self() pointer).
          Asyncify.state = Asyncify.State.Normal;
          
          // Keep the runtime alive so that a re-wind can be done later.
          runAndAbortIfError(_asyncify_stop_unwind);
          if (typeof Fibers != 'undefined') {
            Fibers.trampoline();
          }
        }
      },
  whenDone() {
        assert(Asyncify.currData, 'Tried to wait for an async operation when none is in progress.');
        assert(!Asyncify.asyncPromiseHandlers, 'Cannot have multiple async operations in flight at once');
        return new Promise((resolve, reject) => {
          Asyncify.asyncPromiseHandlers = { resolve, reject };
        });
      },
  allocateData() {
        // An asyncify data structure has three fields:
        //  0  current stack pos
        //  4  max stack pos
        //  8  id of function at bottom of the call stack (callStackIdToFunc[id] == wasm func)
        //
        // The Asyncify ABI only interprets the first two fields, the rest is for the runtime.
        // We also embed a stack in the same memory region here, right next to the structure.
        // This struct is also defined as asyncify_data_t in emscripten/fiber.h
        var ptr = _malloc(12 + Asyncify.StackSize);
        Asyncify.setDataHeader(ptr, ptr + 12, Asyncify.StackSize);
        Asyncify.setDataRewindFunc(ptr);
        return ptr;
      },
  setDataHeader(ptr, stack, stackSize) {
        HEAPU32[((ptr)>>2)] = stack;
        HEAPU32[(((ptr)+(4))>>2)] = stack + stackSize;
      },
  setDataRewindFunc(ptr) {
        var bottomOfCallStack = Asyncify.exportCallStack[0];
        assert(bottomOfCallStack, 'exportCallStack is empty');
        var rewindId = Asyncify.getCallStackId(bottomOfCallStack);
        HEAP32[(((ptr)+(8))>>2)] = rewindId;
      },
  getDataRewindFunc(ptr) {
        var id = HEAP32[(((ptr)+(8))>>2)];
        var func = Asyncify.callStackIdToFunc.get(id);
        assert(func, `id ${id} not found in callStackIdToFunc`);
        return func;
      },
  doRewind(ptr) {
        var original = Asyncify.getDataRewindFunc(ptr);
        var func = Asyncify.funcWrappers.get(original);
        assert(original);
        assert(func);
        // Once we have rewound and the stack we no longer need to artificially
        // keep the runtime alive.
        
        return callUserCallback(func);
      },
  handleSleep(startAsync) {
        assert(Asyncify.state !== Asyncify.State.Disabled, 'Asyncify cannot be done during or after the runtime exits');
        if (ABORT) return;
        if (Asyncify.state === Asyncify.State.Normal) {
          // Prepare to sleep. Call startAsync, and see what happens:
          // if the code decided to call our callback synchronously,
          // then no async operation was in fact begun, and we don't
          // need to do anything.
          var reachedCallback = false;
          var reachedAfterCallback = false;
          startAsync((handleSleepReturnValue = 0) => {
            // old emterpretify API supported other stuff
            assert(['undefined', 'number', 'boolean', 'bigint'].includes(typeof handleSleepReturnValue), `invalid type for handleSleepReturnValue: '${typeof handleSleepReturnValue}'`);
            if (ABORT) return;
            Asyncify.handleSleepReturnValue = handleSleepReturnValue;
            reachedCallback = true;
            if (!reachedAfterCallback) {
              // We are happening synchronously, so no need for async.
              return;
            }
            // This async operation did not happen synchronously, so we did
            // unwind. In that case there can be no compiled code on the stack,
            // as it might break later operations (we can rewind ok now, but if
            // we unwind again, we would unwind through the extra compiled code
            // too).
            assert(!Asyncify.exportCallStack.length, 'Waking up (starting to rewind) must be done from JS, without compiled code on the stack.');
            Asyncify.state = Asyncify.State.Rewinding;
            runAndAbortIfError(() => _asyncify_start_rewind(Asyncify.currData));
            if (typeof MainLoop != 'undefined' && MainLoop.func) {
              MainLoop.resume();
            }
            var asyncWasmReturnValue, isError = false;
            try {
              asyncWasmReturnValue = Asyncify.doRewind(Asyncify.currData);
            } catch (err) {
              asyncWasmReturnValue = err;
              isError = true;
            }
            // Track whether the return value was handled by any promise handlers.
            var handled = false;
            if (!Asyncify.currData) {
              // All asynchronous execution has finished.
              // `asyncWasmReturnValue` now contains the final
              // return value of the exported async WASM function.
              //
              // Note: `asyncWasmReturnValue` is distinct from
              // `Asyncify.handleSleepReturnValue`.
              // `Asyncify.handleSleepReturnValue` contains the return
              // value of the last C function to have executed
              // `Asyncify.handleSleep()`, whereas `asyncWasmReturnValue`
              // contains the return value of the exported WASM function
              // that may have called C functions that
              // call `Asyncify.handleSleep()`.
              var asyncPromiseHandlers = Asyncify.asyncPromiseHandlers;
              if (asyncPromiseHandlers) {
                Asyncify.asyncPromiseHandlers = null;
                (isError ? asyncPromiseHandlers.reject : asyncPromiseHandlers.resolve)(asyncWasmReturnValue);
                handled = true;
              }
            }
            if (isError && !handled) {
              // If there was an error and it was not handled by now, we have no choice but to
              // rethrow that error into the global scope where it can be caught only by
              // `onerror` or `onunhandledpromiserejection`.
              throw asyncWasmReturnValue;
            }
          });
          reachedAfterCallback = true;
          if (!reachedCallback) {
            // A true async operation was begun; start a sleep.
            Asyncify.state = Asyncify.State.Unwinding;
            // TODO: reuse, don't alloc/free every sleep
            Asyncify.currData = Asyncify.allocateData();
            if (typeof MainLoop != 'undefined' && MainLoop.func) {
              MainLoop.pause();
            }
            runAndAbortIfError(() => _asyncify_start_unwind(Asyncify.currData));
          }
        } else if (Asyncify.state === Asyncify.State.Rewinding) {
          // Stop a resume.
          Asyncify.state = Asyncify.State.Normal;
          runAndAbortIfError(_asyncify_stop_rewind);
          _free(Asyncify.currData);
          Asyncify.currData = null;
          // Call all sleep callbacks now that the sleep-resume is all done.
          Asyncify.sleepCallbacks.forEach(callUserCallback);
        } else {
          abort(`invalid state: ${Asyncify.state}`);
        }
        return Asyncify.handleSleepReturnValue;
      },
  handleAsync:(startAsync) => Asyncify.handleSleep(async (wakeUp) => {
        // TODO: add error handling as a second param when handleSleep implements it.
        wakeUp(await startAsync());
      }),
  };
// End JS library code

// include: postlibrary.js
// This file is included after the automatically-generated JS library code
// but before the wasm module is created.

{

  // Begin ATMODULES hooks
  if (Module['noExitRuntime']) noExitRuntime = Module['noExitRuntime'];
if (Module['print']) out = Module['print'];
if (Module['printErr']) err = Module['printErr'];
if (Module['wasmBinary']) wasmBinary = Module['wasmBinary'];

Module['FS_createDataFile'] = FS.createDataFile;
Module['FS_createPreloadedFile'] = FS.createPreloadedFile;

  // End ATMODULES hooks

  checkIncomingModuleAPI();

  if (Module['arguments']) arguments_ = Module['arguments'];
  if (Module['thisProgram']) thisProgram = Module['thisProgram'];

  // Assertions on removed incoming Module JS APIs.
  assert(typeof Module['memoryInitializerPrefixURL'] == 'undefined', 'Module.memoryInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['pthreadMainPrefixURL'] == 'undefined', 'Module.pthreadMainPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['cdInitializerPrefixURL'] == 'undefined', 'Module.cdInitializerPrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['filePackagePrefixURL'] == 'undefined', 'Module.filePackagePrefixURL option was removed, use Module.locateFile instead');
  assert(typeof Module['read'] == 'undefined', 'Module.read option was removed');
  assert(typeof Module['readAsync'] == 'undefined', 'Module.readAsync option was removed (modify readAsync in JS)');
  assert(typeof Module['readBinary'] == 'undefined', 'Module.readBinary option was removed (modify readBinary in JS)');
  assert(typeof Module['setWindowTitle'] == 'undefined', 'Module.setWindowTitle option was removed (modify emscripten_set_window_title in JS)');
  assert(typeof Module['TOTAL_MEMORY'] == 'undefined', 'Module.TOTAL_MEMORY has been renamed Module.INITIAL_MEMORY');
  assert(typeof Module['ENVIRONMENT'] == 'undefined', 'Module.ENVIRONMENT has been deprecated. To force the environment, use the ENVIRONMENT compile-time option (for example, -sENVIRONMENT=web or -sENVIRONMENT=node)');
  assert(typeof Module['STACK_SIZE'] == 'undefined', 'STACK_SIZE can no longer be set at runtime.  Use -sSTACK_SIZE at link time')
  // If memory is defined in wasm, the user can't provide it, or set INITIAL_MEMORY
  assert(typeof Module['wasmMemory'] == 'undefined', 'Use of `wasmMemory` detected.  Use -sIMPORTED_MEMORY to define wasmMemory externally');
  assert(typeof Module['INITIAL_MEMORY'] == 'undefined', 'Detected runtime INITIAL_MEMORY setting.  Use -sIMPORTED_MEMORY to define wasmMemory dynamically');

  if (Module['preInit']) {
    if (typeof Module['preInit'] == 'function') Module['preInit'] = [Module['preInit']];
    while (Module['preInit'].length > 0) {
      Module['preInit'].shift()();
    }
  }
  consumedModuleProp('preInit');
}

// Begin runtime exports
  var missingLibrarySymbols = [
  'writeI53ToI64Clamped',
  'writeI53ToI64Signaling',
  'writeI53ToU64Clamped',
  'writeI53ToU64Signaling',
  'convertI32PairToI53',
  'convertI32PairToI53Checked',
  'convertU32PairToI53',
  'getTempRet0',
  'setTempRet0',
  'zeroMemory',
  'withStackSave',
  'strError',
  'inetPton4',
  'inetNtop4',
  'inetPton6',
  'inetNtop6',
  'readSockaddr',
  'writeSockaddr',
  'readEmAsmArgs',
  'jstoi_q',
  'getExecutableName',
  'autoResumeAudioContext',
  'getDynCaller',
  'asyncLoad',
  'asmjsMangle',
  'mmapAlloc',
  'HandleAllocator',
  'getUniqueRunDependency',
  'addOnInit',
  'addOnPostCtor',
  'addOnPreMain',
  'STACK_SIZE',
  'STACK_ALIGN',
  'POINTER_SIZE',
  'ASSERTIONS',
  'ccall',
  'cwrap',
  'convertJsFunctionToWasm',
  'getEmptyTableSlot',
  'updateTableMap',
  'getFunctionAddress',
  'addFunction',
  'removeFunction',
  'intArrayFromString',
  'intArrayToString',
  'AsciiToString',
  'stringToAscii',
  'UTF16ToString',
  'stringToUTF16',
  'lengthBytesUTF16',
  'UTF32ToString',
  'stringToUTF32',
  'lengthBytesUTF32',
  'writeArrayToMemory',
  'registerFocusEventCallback',
  'fillDeviceOrientationEventData',
  'registerDeviceOrientationEventCallback',
  'fillDeviceMotionEventData',
  'registerDeviceMotionEventCallback',
  'screenOrientation',
  'fillOrientationChangeEventData',
  'registerOrientationChangeEventCallback',
  'fillFullscreenChangeEventData',
  'registerFullscreenChangeEventCallback',
  'JSEvents_requestFullscreen',
  'JSEvents_resizeCanvasForFullscreen',
  'registerRestoreOldStyle',
  'hideEverythingExceptGivenElement',
  'restoreHiddenElements',
  'setLetterbox',
  'softFullscreenResizeWebGLRenderTarget',
  'doRequestFullscreen',
  'fillPointerlockChangeEventData',
  'registerPointerlockChangeEventCallback',
  'registerPointerlockErrorEventCallback',
  'requestPointerLock',
  'fillVisibilityChangeEventData',
  'registerVisibilityChangeEventCallback',
  'registerTouchEventCallback',
  'fillGamepadEventData',
  'registerGamepadEventCallback',
  'registerBeforeUnloadEventCallback',
  'fillBatteryEventData',
  'registerBatteryEventCallback',
  'setCanvasElementSize',
  'getCanvasElementSize',
  'jsStackTrace',
  'getCallstack',
  'convertPCtoSourceLocation',
  'getEnvStrings',
  'checkWasiClock',
  'wasiRightsToMuslOFlags',
  'wasiOFlagsToMuslOFlags',
  'initRandomFill',
  'randomFill',
  'safeSetTimeout',
  'setImmediateWrapped',
  'safeRequestAnimationFrame',
  'clearImmediateWrapped',
  'registerPostMainLoop',
  'registerPreMainLoop',
  'getPromise',
  'makePromise',
  'idsToPromises',
  'makePromiseCallback',
  'ExceptionInfo',
  'findMatchingCatch',
  'incrementUncaughtExceptionCount',
  'decrementUncaughtExceptionCount',
  'Browser_asyncPrepareDataCounter',
  'isLeapYear',
  'ydayFromDate',
  'arraySum',
  'addDays',
  'getSocketFromFD',
  'getSocketAddress',
  'FS_createPreloadedFile',
  'FS_preloadFile',
  'FS_modeStringToFlags',
  'FS_getMode',
  'FS_fileDataToTypedArray',
  'FS_stdin_getChar',
  'FS_mkdirTree',
  '_setNetworkCallback',
  'heapObjectForWebGLType',
  'toTypedArrayIndex',
  'webgl_enable_ANGLE_instanced_arrays',
  'webgl_enable_OES_vertex_array_object',
  'webgl_enable_WEBGL_draw_buffers',
  'webgl_enable_WEBGL_multi_draw',
  'webgl_enable_EXT_polygon_offset_clamp',
  'webgl_enable_EXT_clip_control',
  'webgl_enable_WEBGL_polygon_mode',
  'emscriptenWebGLGet',
  'computeUnpackAlignedImageSize',
  'colorChannelsInGlTextureFormat',
  'emscriptenWebGLGetTexPixelData',
  'emscriptenWebGLGetUniform',
  'webglGetUniformLocation',
  'webglPrepareUniformLocationsBeforeFirstUse',
  'webglGetLeftBracePos',
  'emscriptenWebGLGetVertexAttrib',
  '__glGetActiveAttribOrUniform',
  'writeGLArray',
  'registerWebGlEventCallback',
  'ALLOC_NORMAL',
  'ALLOC_STACK',
  'allocate',
  'writeStringToMemory',
  'writeAsciiToMemory',
  'allocateUTF8',
  'allocateUTF8OnStack',
  'demangle',
  'stackTrace',
  'getNativeTypeSize',
];
missingLibrarySymbols.forEach(missingLibrarySymbol)

  var unexportedSymbols = [
  'run',
  'out',
  'err',
  'callMain',
  'abort',
  'wasmExports',
  'writeStackCookie',
  'checkStackCookie',
  'writeI53ToI64',
  'readI53FromI64',
  'readI53FromU64',
  'INT53_MAX',
  'INT53_MIN',
  'bigintToI53Checked',
  'HEAP8',
  'HEAPU8',
  'HEAP16',
  'HEAPU16',
  'HEAP32',
  'HEAPU32',
  'HEAPF32',
  'HEAPF64',
  'HEAP64',
  'HEAPU64',
  'stackSave',
  'stackRestore',
  'stackAlloc',
  'createNamedFunction',
  'ptrToString',
  'exitJS',
  'getHeapMax',
  'growMemory',
  'ENV',
  'ERRNO_CODES',
  'DNS',
  'Protocols',
  'Sockets',
  'timers',
  'warnOnce',
  'readEmAsmArgsArray',
  'dynCallLegacy',
  'dynCall',
  'handleException',
  'keepRuntimeAlive',
  'runtimeKeepalivePush',
  'runtimeKeepalivePop',
  'callUserCallback',
  'maybeExit',
  'alignMemory',
  'wasmTable',
  'wasmMemory',
  'noExitRuntime',
  'addRunDependency',
  'removeRunDependency',
  'addOnPreRun',
  'addOnExit',
  'addOnPostRun',
  'freeTableIndexes',
  'functionsInTableMap',
  'setValue',
  'getValue',
  'PATH',
  'PATH_FS',
  'UTF8Decoder',
  'UTF8ArrayToString',
  'UTF8ToString',
  'stringToUTF8Array',
  'stringToUTF8',
  'lengthBytesUTF8',
  'UTF16Decoder',
  'stringToNewUTF8',
  'stringToUTF8OnStack',
  'JSEvents',
  'registerKeyEventCallback',
  'specialHTMLTargets',
  'maybeCStringToJsString',
  'findEventTarget',
  'findCanvasEventTarget',
  'getBoundingClientRect',
  'fillMouseEventData',
  'registerMouseEventCallback',
  'registerWheelEventCallback',
  'registerUiEventCallback',
  'currentFullscreenStrategy',
  'restoreOldWindowedStyle',
  'UNWIND_CACHE',
  'ExitStatus',
  'flush_NO_FILESYSTEM',
  'emSetImmediate',
  'emClearImmediate_deps',
  'emClearImmediate',
  'promiseMap',
  'uncaughtExceptionCount',
  'exceptionCaught',
  'Browser',
  'requestFullscreen',
  'requestFullScreen',
  'setCanvasSize',
  'getUserMedia',
  'createContext',
  'getPreloadedImageData__data',
  'wget',
  'MONTH_DAYS_REGULAR',
  'MONTH_DAYS_LEAP',
  'MONTH_DAYS_REGULAR_CUMULATIVE',
  'MONTH_DAYS_LEAP_CUMULATIVE',
  'SYSCALLS',
  'preloadPlugins',
  'FS_stdin_getChar_buffer',
  'FS_unlink',
  'FS_createPath',
  'FS_createDevice',
  'FS_readFile',
  'FS',
  'FS_root',
  'FS_mounts',
  'FS_devices',
  'FS_streams',
  'FS_nextInode',
  'FS_nameTable',
  'FS_currentPath',
  'FS_initialized',
  'FS_ignorePermissions',
  'FS_filesystems',
  'FS_syncFSRequests',
  'FS_lookupPath',
  'FS_getPath',
  'FS_hashName',
  'FS_hashAddNode',
  'FS_hashRemoveNode',
  'FS_lookupNode',
  'FS_createNode',
  'FS_destroyNode',
  'FS_isRoot',
  'FS_isMountpoint',
  'FS_isFile',
  'FS_isDir',
  'FS_isLink',
  'FS_isChrdev',
  'FS_isBlkdev',
  'FS_isFIFO',
  'FS_isSocket',
  'FS_flagsToPermissionString',
  'FS_nodePermissions',
  'FS_mayLookup',
  'FS_mayCreate',
  'FS_mayDelete',
  'FS_mayOpen',
  'FS_checkOpExists',
  'FS_nextfd',
  'FS_getStreamChecked',
  'FS_getStream',
  'FS_createStream',
  'FS_closeStream',
  'FS_dupStream',
  'FS_doSetAttr',
  'FS_chrdev_stream_ops',
  'FS_major',
  'FS_minor',
  'FS_makedev',
  'FS_registerDevice',
  'FS_getDevice',
  'FS_getMounts',
  'FS_syncfs',
  'FS_mount',
  'FS_unmount',
  'FS_lookup',
  'FS_mknod',
  'FS_statfs',
  'FS_statfsStream',
  'FS_statfsNode',
  'FS_create',
  'FS_mkdir',
  'FS_mkdev',
  'FS_symlink',
  'FS_rename',
  'FS_rmdir',
  'FS_readdir',
  'FS_readlink',
  'FS_stat',
  'FS_fstat',
  'FS_lstat',
  'FS_doChmod',
  'FS_chmod',
  'FS_lchmod',
  'FS_fchmod',
  'FS_doChown',
  'FS_chown',
  'FS_lchown',
  'FS_fchown',
  'FS_doTruncate',
  'FS_truncate',
  'FS_ftruncate',
  'FS_utime',
  'FS_open',
  'FS_close',
  'FS_isClosed',
  'FS_llseek',
  'FS_read',
  'FS_write',
  'FS_mmap',
  'FS_msync',
  'FS_ioctl',
  'FS_writeFile',
  'FS_cwd',
  'FS_chdir',
  'FS_createDefaultDirectories',
  'FS_createDefaultDevices',
  'FS_createSpecialDirectories',
  'FS_createStandardStreams',
  'FS_staticInit',
  'FS_init',
  'FS_quit',
  'FS_findObject',
  'FS_analyzePath',
  'FS_createFile',
  'FS_createDataFile',
  'FS_forceLoadFile',
  'FS_createLazyFile',
  'MEMFS',
  'TTY',
  'PIPEFS',
  'SOCKFS',
  'tempFixedLengthArray',
  'miniTempWebGLFloatBuffers',
  'miniTempWebGLIntBuffers',
  'GL',
  'AL',
  'GLUT',
  'EGL',
  'GLEW',
  'IDBStore',
  'runAndAbortIfError',
  'Asyncify',
  'Fibers',
  'SDL',
  'SDL_gfx',
  'print',
  'printErr',
  'jstoi_s',
  'WebGPU',
  'emwgpuStringToInt_BufferMapState',
  'emwgpuStringToInt_CompilationMessageType',
  'emwgpuStringToInt_DeviceLostReason',
  'emwgpuStringToInt_FeatureName',
  'emwgpuStringToInt_PreferredFormat',
];
unexportedSymbols.forEach(unexportedRuntimeSymbol);

  // End runtime exports
  // Begin JS library exports
  // End JS library exports

// end include: postlibrary.js

function checkIncomingModuleAPI() {
  ignoredModuleProp('fetchSettings');
  ignoredModuleProp('logReadFiles');
  ignoredModuleProp('loadSplitModule');
  ignoredModuleProp('onMalloc');
  ignoredModuleProp('onRealloc');
  ignoredModuleProp('onFree');
  ignoredModuleProp('onSbrkGrow');
}
function mira_open_image_picker(doc_width,doc_height) { const targetW = Math.max(1, doc_width | 0); const targetH = Math.max(1, doc_height | 0); if (!Module.miraImageInput) { const input = document.createElement("input"); input.type = "file"; input.accept = "image/*"; input.style.display = "none"; document.body.appendChild(input); Module.miraImageInput = input; } const input = Module.miraImageInput; input.onchange = async function() { const file = input.files && input.files[0]; if (!file) { return; } let bitmap = null; try { bitmap = await createImageBitmap(file); const canvas = typeof OffscreenCanvas !== 'undefined' ? new OffscreenCanvas(targetW, targetH) : document.createElement("canvas"); canvas.width = targetW; canvas.height = targetH; const ctx = canvas.getContext("2d", { willReadFrequently: true }); ctx.fillStyle = "#fff"; ctx.fillRect(0, 0, targetW, targetH); ctx.imageSmoothingEnabled = true; ctx.imageSmoothingQuality = "high"; const scale = Math.min(targetW / Math.max(1, bitmap.width), targetH / Math.max(1, bitmap.height)); const w = Math.max(1, Math.floor(bitmap.width * scale)); const h = Math.max(1, Math.floor(bitmap.height * scale)); const x = Math.floor((targetW - w) * 0.5); const y = Math.floor((targetH - h) * 0.5); ctx.drawImage(bitmap, x, y, w, h); const rgba = ctx.getImageData(0, 0, targetW, targetH).data; const bayer = [ 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5, ]; const pixels = new Uint8Array(targetW * targetH); for (let p = 0, r = 0; p < pixels.length; ++p, r += 4) { const alpha = rgba[r + 3] / 255.0; const luma = ((rgba[r] * 0.299) + (rgba[r + 1] * 0.587) + (rgba[r + 2] * 0.114)) * alpha + (255.0 * (1.0 - alpha)); const px = p % targetW; const py = Math.floor(p / targetW); const rank = bayer[((py & 3) * 4) + (px & 3)]; const threshold = ((rank + 0.5) / 16.0) * 255.0; pixels[p] = luma >= threshold ? 255 : 0; } const data = _malloc(pixels.length); HEAPU8.set(pixels, data); const filename = file.name || "Image"; const dot = filename.lastIndexOf("."); const stem = (dot > 0 ? filename.slice(0, dot) : filename) || "Image"; const nameLen = lengthBytesUTF8(stem) + 1; const name = _malloc(nameLen); stringToUTF8(stem, name, nameLen); _mira_import_image_ready(targetW, targetH, data, pixels.length, name); _free(name); _free(data); } catch (error) { } finally { if (bitmap && bitmap.close) { bitmap.close(); } input.value = ""; } }; input.value = ""; input.click(); }
function mira_download_png(width,height,rgba,byte_count) { const w = Math.max(1, width | 0); const h = Math.max(1, height | 0); const expected = w * h * 4; if (!rgba || byte_count < expected) { return; } const canvas = document.createElement("canvas"); canvas.width = w; canvas.height = h; const ctx = canvas.getContext("2d"); const copy = new Uint8ClampedArray(HEAPU8.subarray(rgba, rgba + expected)); ctx.putImageData(new ImageData(copy, w, h), 0, 0); canvas.toBlob(function(blob) { if (!blob) { return; } const url = URL.createObjectURL(blob); const link = document.createElement("a"); link.href = url; link.download = "mira.png"; document.body.appendChild(link); link.click(); link.remove(); setTimeout(function() { URL.revokeObjectURL(url); }, 0); }, "image/png"); }
function mira_export_failed() { console.error("placeholder"); }

// Imports from the Wasm binary.
var _mira_import_image_ready = Module['_mira_import_image_ready'] = makeInvalidEarlyAccess('_mira_import_image_ready');
var _main = Module['_main'] = makeInvalidEarlyAccess('_main');
var _emwgpuCreateBindGroup = makeInvalidEarlyAccess('_emwgpuCreateBindGroup');
var _emwgpuCreateBindGroupLayout = makeInvalidEarlyAccess('_emwgpuCreateBindGroupLayout');
var _emwgpuCreateCommandBuffer = makeInvalidEarlyAccess('_emwgpuCreateCommandBuffer');
var _emwgpuCreateCommandEncoder = makeInvalidEarlyAccess('_emwgpuCreateCommandEncoder');
var _emwgpuCreateComputePassEncoder = makeInvalidEarlyAccess('_emwgpuCreateComputePassEncoder');
var _emwgpuCreateComputePipeline = makeInvalidEarlyAccess('_emwgpuCreateComputePipeline');
var _emwgpuCreateExternalTexture = makeInvalidEarlyAccess('_emwgpuCreateExternalTexture');
var _emwgpuCreatePipelineLayout = makeInvalidEarlyAccess('_emwgpuCreatePipelineLayout');
var _emwgpuCreateQuerySet = makeInvalidEarlyAccess('_emwgpuCreateQuerySet');
var _emwgpuCreateRenderBundle = makeInvalidEarlyAccess('_emwgpuCreateRenderBundle');
var _emwgpuCreateRenderBundleEncoder = makeInvalidEarlyAccess('_emwgpuCreateRenderBundleEncoder');
var _emwgpuCreateRenderPassEncoder = makeInvalidEarlyAccess('_emwgpuCreateRenderPassEncoder');
var _emwgpuCreateRenderPipeline = makeInvalidEarlyAccess('_emwgpuCreateRenderPipeline');
var _emwgpuCreateSampler = makeInvalidEarlyAccess('_emwgpuCreateSampler');
var _emwgpuCreateSurface = makeInvalidEarlyAccess('_emwgpuCreateSurface');
var _emwgpuCreateTexture = makeInvalidEarlyAccess('_emwgpuCreateTexture');
var _emwgpuCreateTextureView = makeInvalidEarlyAccess('_emwgpuCreateTextureView');
var _emwgpuCreateAdapter = makeInvalidEarlyAccess('_emwgpuCreateAdapter');
var _emwgpuImportBuffer = makeInvalidEarlyAccess('_emwgpuImportBuffer');
var _emwgpuCreateDevice = makeInvalidEarlyAccess('_emwgpuCreateDevice');
var _emwgpuCreateQueue = makeInvalidEarlyAccess('_emwgpuCreateQueue');
var _emwgpuCreateShaderModule = makeInvalidEarlyAccess('_emwgpuCreateShaderModule');
var _emwgpuOnCompilationInfoCompleted = makeInvalidEarlyAccess('_emwgpuOnCompilationInfoCompleted');
var _free = Module['_free'] = makeInvalidEarlyAccess('_free');
var _emwgpuOnCreateComputePipelineCompleted = makeInvalidEarlyAccess('_emwgpuOnCreateComputePipelineCompleted');
var _emwgpuOnCreateRenderPipelineCompleted = makeInvalidEarlyAccess('_emwgpuOnCreateRenderPipelineCompleted');
var _emwgpuOnDeviceLostCompleted = makeInvalidEarlyAccess('_emwgpuOnDeviceLostCompleted');
var _emwgpuOnMapAsyncCompleted = makeInvalidEarlyAccess('_emwgpuOnMapAsyncCompleted');
var _emwgpuOnPopErrorScopeCompleted = makeInvalidEarlyAccess('_emwgpuOnPopErrorScopeCompleted');
var _emwgpuOnRequestAdapterCompleted = makeInvalidEarlyAccess('_emwgpuOnRequestAdapterCompleted');
var _emwgpuOnRequestDeviceCompleted = makeInvalidEarlyAccess('_emwgpuOnRequestDeviceCompleted');
var _emwgpuOnWorkDoneCompleted = makeInvalidEarlyAccess('_emwgpuOnWorkDoneCompleted');
var _emwgpuOnUncapturedError = makeInvalidEarlyAccess('_emwgpuOnUncapturedError');
var _fflush = makeInvalidEarlyAccess('_fflush');
var _emscripten_stack_get_end = makeInvalidEarlyAccess('_emscripten_stack_get_end');
var _emscripten_stack_get_base = makeInvalidEarlyAccess('_emscripten_stack_get_base');
var _strerror = makeInvalidEarlyAccess('_strerror');
var _malloc = Module['_malloc'] = makeInvalidEarlyAccess('_malloc');
var _memalign = makeInvalidEarlyAccess('_memalign');
var _emscripten_stack_init = makeInvalidEarlyAccess('_emscripten_stack_init');
var _emscripten_stack_get_free = makeInvalidEarlyAccess('_emscripten_stack_get_free');
var __emscripten_stack_restore = makeInvalidEarlyAccess('__emscripten_stack_restore');
var __emscripten_stack_alloc = makeInvalidEarlyAccess('__emscripten_stack_alloc');
var _emscripten_stack_get_current = makeInvalidEarlyAccess('_emscripten_stack_get_current');
var dynCall_viiii = makeInvalidEarlyAccess('dynCall_viiii');
var dynCall_idi = makeInvalidEarlyAccess('dynCall_idi');
var dynCall_vi = makeInvalidEarlyAccess('dynCall_vi');
var dynCall_viiiii = makeInvalidEarlyAccess('dynCall_viiiii');
var dynCall_iiii = makeInvalidEarlyAccess('dynCall_iiii');
var dynCall_ii = makeInvalidEarlyAccess('dynCall_ii');
var dynCall_viji = makeInvalidEarlyAccess('dynCall_viji');
var dynCall_jiji = makeInvalidEarlyAccess('dynCall_jiji');
var dynCall_iidiiii = makeInvalidEarlyAccess('dynCall_iidiiii');
var dynCall_vii = makeInvalidEarlyAccess('dynCall_vii');
var _asyncify_start_unwind = makeInvalidEarlyAccess('_asyncify_start_unwind');
var _asyncify_stop_unwind = makeInvalidEarlyAccess('_asyncify_stop_unwind');
var _asyncify_start_rewind = makeInvalidEarlyAccess('_asyncify_start_rewind');
var _asyncify_stop_rewind = makeInvalidEarlyAccess('_asyncify_stop_rewind');
var memory = makeInvalidEarlyAccess('memory');
var __indirect_function_table = makeInvalidEarlyAccess('__indirect_function_table');
var wasmMemory = makeInvalidEarlyAccess('wasmMemory');

function assignWasmExports(wasmExports) {
  assert(typeof wasmExports['mira_import_image_ready'] != 'undefined', 'missing Wasm export: mira_import_image_ready');
  assert(typeof wasmExports['main'] != 'undefined', 'missing Wasm export: main');
  assert(typeof wasmExports['emwgpuCreateBindGroup'] != 'undefined', 'missing Wasm export: emwgpuCreateBindGroup');
  assert(typeof wasmExports['emwgpuCreateBindGroupLayout'] != 'undefined', 'missing Wasm export: emwgpuCreateBindGroupLayout');
  assert(typeof wasmExports['emwgpuCreateCommandBuffer'] != 'undefined', 'missing Wasm export: emwgpuCreateCommandBuffer');
  assert(typeof wasmExports['emwgpuCreateCommandEncoder'] != 'undefined', 'missing Wasm export: emwgpuCreateCommandEncoder');
  assert(typeof wasmExports['emwgpuCreateComputePassEncoder'] != 'undefined', 'missing Wasm export: emwgpuCreateComputePassEncoder');
  assert(typeof wasmExports['emwgpuCreateComputePipeline'] != 'undefined', 'missing Wasm export: emwgpuCreateComputePipeline');
  assert(typeof wasmExports['emwgpuCreateExternalTexture'] != 'undefined', 'missing Wasm export: emwgpuCreateExternalTexture');
  assert(typeof wasmExports['emwgpuCreatePipelineLayout'] != 'undefined', 'missing Wasm export: emwgpuCreatePipelineLayout');
  assert(typeof wasmExports['emwgpuCreateQuerySet'] != 'undefined', 'missing Wasm export: emwgpuCreateQuerySet');
  assert(typeof wasmExports['emwgpuCreateRenderBundle'] != 'undefined', 'missing Wasm export: emwgpuCreateRenderBundle');
  assert(typeof wasmExports['emwgpuCreateRenderBundleEncoder'] != 'undefined', 'missing Wasm export: emwgpuCreateRenderBundleEncoder');
  assert(typeof wasmExports['emwgpuCreateRenderPassEncoder'] != 'undefined', 'missing Wasm export: emwgpuCreateRenderPassEncoder');
  assert(typeof wasmExports['emwgpuCreateRenderPipeline'] != 'undefined', 'missing Wasm export: emwgpuCreateRenderPipeline');
  assert(typeof wasmExports['emwgpuCreateSampler'] != 'undefined', 'missing Wasm export: emwgpuCreateSampler');
  assert(typeof wasmExports['emwgpuCreateSurface'] != 'undefined', 'missing Wasm export: emwgpuCreateSurface');
  assert(typeof wasmExports['emwgpuCreateTexture'] != 'undefined', 'missing Wasm export: emwgpuCreateTexture');
  assert(typeof wasmExports['emwgpuCreateTextureView'] != 'undefined', 'missing Wasm export: emwgpuCreateTextureView');
  assert(typeof wasmExports['emwgpuCreateAdapter'] != 'undefined', 'missing Wasm export: emwgpuCreateAdapter');
  assert(typeof wasmExports['emwgpuImportBuffer'] != 'undefined', 'missing Wasm export: emwgpuImportBuffer');
  assert(typeof wasmExports['emwgpuCreateDevice'] != 'undefined', 'missing Wasm export: emwgpuCreateDevice');
  assert(typeof wasmExports['emwgpuCreateQueue'] != 'undefined', 'missing Wasm export: emwgpuCreateQueue');
  assert(typeof wasmExports['emwgpuCreateShaderModule'] != 'undefined', 'missing Wasm export: emwgpuCreateShaderModule');
  assert(typeof wasmExports['emwgpuOnCompilationInfoCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnCompilationInfoCompleted');
  assert(typeof wasmExports['free'] != 'undefined', 'missing Wasm export: free');
  assert(typeof wasmExports['emwgpuOnCreateComputePipelineCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnCreateComputePipelineCompleted');
  assert(typeof wasmExports['emwgpuOnCreateRenderPipelineCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnCreateRenderPipelineCompleted');
  assert(typeof wasmExports['emwgpuOnDeviceLostCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnDeviceLostCompleted');
  assert(typeof wasmExports['emwgpuOnMapAsyncCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnMapAsyncCompleted');
  assert(typeof wasmExports['emwgpuOnPopErrorScopeCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnPopErrorScopeCompleted');
  assert(typeof wasmExports['emwgpuOnRequestAdapterCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnRequestAdapterCompleted');
  assert(typeof wasmExports['emwgpuOnRequestDeviceCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnRequestDeviceCompleted');
  assert(typeof wasmExports['emwgpuOnWorkDoneCompleted'] != 'undefined', 'missing Wasm export: emwgpuOnWorkDoneCompleted');
  assert(typeof wasmExports['emwgpuOnUncapturedError'] != 'undefined', 'missing Wasm export: emwgpuOnUncapturedError');
  assert(typeof wasmExports['fflush'] != 'undefined', 'missing Wasm export: fflush');
  assert(typeof wasmExports['emscripten_stack_get_end'] != 'undefined', 'missing Wasm export: emscripten_stack_get_end');
  assert(typeof wasmExports['emscripten_stack_get_base'] != 'undefined', 'missing Wasm export: emscripten_stack_get_base');
  assert(typeof wasmExports['strerror'] != 'undefined', 'missing Wasm export: strerror');
  assert(typeof wasmExports['malloc'] != 'undefined', 'missing Wasm export: malloc');
  assert(typeof wasmExports['memalign'] != 'undefined', 'missing Wasm export: memalign');
  assert(typeof wasmExports['emscripten_stack_init'] != 'undefined', 'missing Wasm export: emscripten_stack_init');
  assert(typeof wasmExports['emscripten_stack_get_free'] != 'undefined', 'missing Wasm export: emscripten_stack_get_free');
  assert(typeof wasmExports['_emscripten_stack_restore'] != 'undefined', 'missing Wasm export: _emscripten_stack_restore');
  assert(typeof wasmExports['_emscripten_stack_alloc'] != 'undefined', 'missing Wasm export: _emscripten_stack_alloc');
  assert(typeof wasmExports['emscripten_stack_get_current'] != 'undefined', 'missing Wasm export: emscripten_stack_get_current');
  assert(typeof wasmExports['dynCall_viiii'] != 'undefined', 'missing Wasm export: dynCall_viiii');
  assert(typeof wasmExports['dynCall_idi'] != 'undefined', 'missing Wasm export: dynCall_idi');
  assert(typeof wasmExports['dynCall_vi'] != 'undefined', 'missing Wasm export: dynCall_vi');
  assert(typeof wasmExports['dynCall_viiiii'] != 'undefined', 'missing Wasm export: dynCall_viiiii');
  assert(typeof wasmExports['dynCall_iiii'] != 'undefined', 'missing Wasm export: dynCall_iiii');
  assert(typeof wasmExports['dynCall_ii'] != 'undefined', 'missing Wasm export: dynCall_ii');
  assert(typeof wasmExports['dynCall_viji'] != 'undefined', 'missing Wasm export: dynCall_viji');
  assert(typeof wasmExports['dynCall_jiji'] != 'undefined', 'missing Wasm export: dynCall_jiji');
  assert(typeof wasmExports['dynCall_iidiiii'] != 'undefined', 'missing Wasm export: dynCall_iidiiii');
  assert(typeof wasmExports['dynCall_vii'] != 'undefined', 'missing Wasm export: dynCall_vii');
  assert(typeof wasmExports['asyncify_start_unwind'] != 'undefined', 'missing Wasm export: asyncify_start_unwind');
  assert(typeof wasmExports['asyncify_stop_unwind'] != 'undefined', 'missing Wasm export: asyncify_stop_unwind');
  assert(typeof wasmExports['asyncify_start_rewind'] != 'undefined', 'missing Wasm export: asyncify_start_rewind');
  assert(typeof wasmExports['asyncify_stop_rewind'] != 'undefined', 'missing Wasm export: asyncify_stop_rewind');
  assert(typeof wasmExports['memory'] != 'undefined', 'missing Wasm export: memory');
  assert(typeof wasmExports['__indirect_function_table'] != 'undefined', 'missing Wasm export: __indirect_function_table');
  _mira_import_image_ready = Module['_mira_import_image_ready'] = createExportWrapper('mira_import_image_ready', 5);
  _main = Module['_main'] = createExportWrapper('main', 2);
  _emwgpuCreateBindGroup = createExportWrapper('emwgpuCreateBindGroup', 1);
  _emwgpuCreateBindGroupLayout = createExportWrapper('emwgpuCreateBindGroupLayout', 1);
  _emwgpuCreateCommandBuffer = createExportWrapper('emwgpuCreateCommandBuffer', 1);
  _emwgpuCreateCommandEncoder = createExportWrapper('emwgpuCreateCommandEncoder', 1);
  _emwgpuCreateComputePassEncoder = createExportWrapper('emwgpuCreateComputePassEncoder', 1);
  _emwgpuCreateComputePipeline = createExportWrapper('emwgpuCreateComputePipeline', 1);
  _emwgpuCreateExternalTexture = createExportWrapper('emwgpuCreateExternalTexture', 1);
  _emwgpuCreatePipelineLayout = createExportWrapper('emwgpuCreatePipelineLayout', 1);
  _emwgpuCreateQuerySet = createExportWrapper('emwgpuCreateQuerySet', 1);
  _emwgpuCreateRenderBundle = createExportWrapper('emwgpuCreateRenderBundle', 1);
  _emwgpuCreateRenderBundleEncoder = createExportWrapper('emwgpuCreateRenderBundleEncoder', 1);
  _emwgpuCreateRenderPassEncoder = createExportWrapper('emwgpuCreateRenderPassEncoder', 1);
  _emwgpuCreateRenderPipeline = createExportWrapper('emwgpuCreateRenderPipeline', 1);
  _emwgpuCreateSampler = createExportWrapper('emwgpuCreateSampler', 1);
  _emwgpuCreateSurface = createExportWrapper('emwgpuCreateSurface', 1);
  _emwgpuCreateTexture = createExportWrapper('emwgpuCreateTexture', 1);
  _emwgpuCreateTextureView = createExportWrapper('emwgpuCreateTextureView', 1);
  _emwgpuCreateAdapter = createExportWrapper('emwgpuCreateAdapter', 1);
  _emwgpuImportBuffer = createExportWrapper('emwgpuImportBuffer', 1);
  _emwgpuCreateDevice = createExportWrapper('emwgpuCreateDevice', 2);
  _emwgpuCreateQueue = createExportWrapper('emwgpuCreateQueue', 1);
  _emwgpuCreateShaderModule = createExportWrapper('emwgpuCreateShaderModule', 1);
  _emwgpuOnCompilationInfoCompleted = createExportWrapper('emwgpuOnCompilationInfoCompleted', 3);
  _free = Module['_free'] = createExportWrapper('free', 1);
  _emwgpuOnCreateComputePipelineCompleted = createExportWrapper('emwgpuOnCreateComputePipelineCompleted', 4);
  _emwgpuOnCreateRenderPipelineCompleted = createExportWrapper('emwgpuOnCreateRenderPipelineCompleted', 4);
  _emwgpuOnDeviceLostCompleted = createExportWrapper('emwgpuOnDeviceLostCompleted', 3);
  _emwgpuOnMapAsyncCompleted = createExportWrapper('emwgpuOnMapAsyncCompleted', 3);
  _emwgpuOnPopErrorScopeCompleted = createExportWrapper('emwgpuOnPopErrorScopeCompleted', 4);
  _emwgpuOnRequestAdapterCompleted = createExportWrapper('emwgpuOnRequestAdapterCompleted', 4);
  _emwgpuOnRequestDeviceCompleted = createExportWrapper('emwgpuOnRequestDeviceCompleted', 4);
  _emwgpuOnWorkDoneCompleted = createExportWrapper('emwgpuOnWorkDoneCompleted', 2);
  _emwgpuOnUncapturedError = createExportWrapper('emwgpuOnUncapturedError', 3);
  _fflush = createExportWrapper('fflush', 1);
  _emscripten_stack_get_end = wasmExports['emscripten_stack_get_end'];
  _emscripten_stack_get_base = wasmExports['emscripten_stack_get_base'];
  _strerror = createExportWrapper('strerror', 1);
  _malloc = Module['_malloc'] = createExportWrapper('malloc', 1);
  _memalign = createExportWrapper('memalign', 2);
  _emscripten_stack_init = wasmExports['emscripten_stack_init'];
  _emscripten_stack_get_free = wasmExports['emscripten_stack_get_free'];
  __emscripten_stack_restore = wasmExports['_emscripten_stack_restore'];
  __emscripten_stack_alloc = wasmExports['_emscripten_stack_alloc'];
  _emscripten_stack_get_current = wasmExports['emscripten_stack_get_current'];
  dynCall_viiii = dynCalls['viiii'] = createExportWrapper('dynCall_viiii', 5);
  dynCall_idi = dynCalls['idi'] = createExportWrapper('dynCall_idi', 3);
  dynCall_vi = dynCalls['vi'] = createExportWrapper('dynCall_vi', 2);
  dynCall_viiiii = dynCalls['viiiii'] = createExportWrapper('dynCall_viiiii', 6);
  dynCall_iiii = dynCalls['iiii'] = createExportWrapper('dynCall_iiii', 4);
  dynCall_ii = dynCalls['ii'] = createExportWrapper('dynCall_ii', 2);
  dynCall_viji = dynCalls['viji'] = createExportWrapper('dynCall_viji', 4);
  dynCall_jiji = dynCalls['jiji'] = createExportWrapper('dynCall_jiji', 4);
  dynCall_iidiiii = dynCalls['iidiiii'] = createExportWrapper('dynCall_iidiiii', 7);
  dynCall_vii = dynCalls['vii'] = createExportWrapper('dynCall_vii', 3);
  _asyncify_start_unwind = createExportWrapper('asyncify_start_unwind', 1);
  _asyncify_stop_unwind = createExportWrapper('asyncify_stop_unwind', 0);
  _asyncify_start_rewind = createExportWrapper('asyncify_start_rewind', 1);
  _asyncify_stop_rewind = createExportWrapper('asyncify_stop_rewind', 0);
  memory = wasmMemory = wasmExports['memory'];
  __indirect_function_table = wasmExports['__indirect_function_table'];
}

var wasmImports = {
  /** @export */
  _abort_js: __abort_js,
  /** @export */
  emscripten_err: _emscripten_err,
  /** @export */
  emscripten_get_device_pixel_ratio: _emscripten_get_device_pixel_ratio,
  /** @export */
  emscripten_get_element_css_size: _emscripten_get_element_css_size,
  /** @export */
  emscripten_has_asyncify: _emscripten_has_asyncify,
  /** @export */
  emscripten_request_animation_frame_loop: _emscripten_request_animation_frame_loop,
  /** @export */
  emscripten_resize_heap: _emscripten_resize_heap,
  /** @export */
  emscripten_run_script: _emscripten_run_script,
  /** @export */
  emscripten_set_canvas_element_size: _emscripten_set_canvas_element_size,
  /** @export */
  emscripten_set_keydown_callback_on_thread: _emscripten_set_keydown_callback_on_thread,
  /** @export */
  emscripten_set_mousedown_callback_on_thread: _emscripten_set_mousedown_callback_on_thread,
  /** @export */
  emscripten_set_mousemove_callback_on_thread: _emscripten_set_mousemove_callback_on_thread,
  /** @export */
  emscripten_set_mouseup_callback_on_thread: _emscripten_set_mouseup_callback_on_thread,
  /** @export */
  emscripten_set_resize_callback_on_thread: _emscripten_set_resize_callback_on_thread,
  /** @export */
  emscripten_set_wheel_callback_on_thread: _emscripten_set_wheel_callback_on_thread,
  /** @export */
  emwgpuAdapterRequestDevice: _emwgpuAdapterRequestDevice,
  /** @export */
  emwgpuBufferGetConstMappedRange: _emwgpuBufferGetConstMappedRange,
  /** @export */
  emwgpuBufferMapAsync: _emwgpuBufferMapAsync,
  /** @export */
  emwgpuBufferUnmap: _emwgpuBufferUnmap,
  /** @export */
  emwgpuDelete: _emwgpuDelete,
  /** @export */
  emwgpuDeviceCreateBuffer: _emwgpuDeviceCreateBuffer,
  /** @export */
  emwgpuDeviceCreateShaderModule: _emwgpuDeviceCreateShaderModule,
  /** @export */
  emwgpuDeviceDestroy: _emwgpuDeviceDestroy,
  /** @export */
  emwgpuGetPreferredFormat: _emwgpuGetPreferredFormat,
  /** @export */
  emwgpuInstanceRequestAdapter: _emwgpuInstanceRequestAdapter,
  /** @export */
  emwgpuWaitAny: _emwgpuWaitAny,
  /** @export */
  fd_close: _fd_close,
  /** @export */
  fd_seek: _fd_seek,
  /** @export */
  fd_write: _fd_write,
  /** @export */
  mira_download_png,
  /** @export */
  mira_export_failed,
  /** @export */
  mira_open_image_picker,
  /** @export */
  wgpuCommandEncoderBeginRenderPass: _wgpuCommandEncoderBeginRenderPass,
  /** @export */
  wgpuCommandEncoderCopyTextureToBuffer: _wgpuCommandEncoderCopyTextureToBuffer,
  /** @export */
  wgpuCommandEncoderFinish: _wgpuCommandEncoderFinish,
  /** @export */
  wgpuDeviceCreateBindGroup: _wgpuDeviceCreateBindGroup,
  /** @export */
  wgpuDeviceCreateBindGroupLayout: _wgpuDeviceCreateBindGroupLayout,
  /** @export */
  wgpuDeviceCreateCommandEncoder: _wgpuDeviceCreateCommandEncoder,
  /** @export */
  wgpuDeviceCreatePipelineLayout: _wgpuDeviceCreatePipelineLayout,
  /** @export */
  wgpuDeviceCreateRenderPipeline: _wgpuDeviceCreateRenderPipeline,
  /** @export */
  wgpuDeviceCreateSampler: _wgpuDeviceCreateSampler,
  /** @export */
  wgpuDeviceCreateTexture: _wgpuDeviceCreateTexture,
  /** @export */
  wgpuInstanceCreateSurface: _wgpuInstanceCreateSurface,
  /** @export */
  wgpuQueueSubmit: _wgpuQueueSubmit,
  /** @export */
  wgpuQueueWriteBuffer: _wgpuQueueWriteBuffer,
  /** @export */
  wgpuQueueWriteTexture: _wgpuQueueWriteTexture,
  /** @export */
  wgpuRenderPassEncoderDraw: _wgpuRenderPassEncoderDraw,
  /** @export */
  wgpuRenderPassEncoderEnd: _wgpuRenderPassEncoderEnd,
  /** @export */
  wgpuRenderPassEncoderSetBindGroup: _wgpuRenderPassEncoderSetBindGroup,
  /** @export */
  wgpuRenderPassEncoderSetPipeline: _wgpuRenderPassEncoderSetPipeline,
  /** @export */
  wgpuRenderPassEncoderSetVertexBuffer: _wgpuRenderPassEncoderSetVertexBuffer,
  /** @export */
  wgpuSurfaceConfigure: _wgpuSurfaceConfigure,
  /** @export */
  wgpuSurfaceGetCurrentTexture: _wgpuSurfaceGetCurrentTexture,
  /** @export */
  wgpuTextureCreateView: _wgpuTextureCreateView
};


// include: postamble.js
// === Auto-generated postamble setup entry stuff ===

var calledRun;

function callMain() {
  assert(runDependencies == 0, 'cannot call main when async dependencies remain! (listen on Module["onRuntimeInitialized"])');
  assert(typeof onPreRuns === 'undefined' || onPreRuns.length == 0, 'cannot call main when preRun functions remain to be called');

  var entryFunction = _main;

  var argc = 0;
  var argv = 0;

  try {

    var ret = entryFunction(argc, argv);

    // if we're not running an evented main loop, it's time to exit
    exitJS(ret, /* implicit = */ true);
    return ret;
  } catch (e) {
    return handleException(e);
  }
}

function stackCheckInit() {
  // This is normally called automatically during __wasm_call_ctors but need to
  // get these values before even running any of the ctors so we call it redundantly
  // here.
  _emscripten_stack_init();
  // TODO(sbc): Move writeStackCookie to native to to avoid this.
  writeStackCookie();
}

function run() {

  if (runDependencies > 0) {
    dependenciesFulfilled = run;
    return;
  }

  stackCheckInit();

  preRun();

  // a preRun added a dependency, run will be called later
  if (runDependencies > 0) {
    dependenciesFulfilled = run;
    return;
  }

  function doRun() {
    // run may have just been called through dependencies being fulfilled just in this very frame,
    // or while the async setStatus time below was happening
    assert(!calledRun);
    calledRun = true;
    Module['calledRun'] = true;

    if (ABORT) return;

    initRuntime();

    preMain();

    Module['onRuntimeInitialized']?.();
    consumedModuleProp('onRuntimeInitialized');

    var noInitialRun = Module['noInitialRun'] || false;
    if (!noInitialRun) callMain();

    postRun();
  }

  if (Module['setStatus']) {
    Module['setStatus']('Running...');
    setTimeout(() => {
      setTimeout(() => Module['setStatus'](''), 1);
      doRun();
    }, 1);
  } else
  {
    doRun();
  }
  checkStackCookie();
}

function checkUnflushedContent() {
  // Compiler settings do not allow exiting the runtime, so flushing
  // the streams is not possible. but in ASSERTIONS mode we check
  // if there was something to flush, and if so tell the user they
  // should request that the runtime be exitable.
  // Normally we would not even include flush() at all, but in ASSERTIONS
  // builds we do so just for this check, and here we see if there is any
  // content to flush, that is, we check if there would have been
  // something a non-ASSERTIONS build would have not seen.
  // How we flush the streams depends on whether we are in SYSCALLS_REQUIRE_FILESYSTEM=0
  // mode (which has its own special function for this; otherwise, all
  // the code is inside libc)
  var oldOut = out;
  var oldErr = err;
  var has = false;
  out = err = (x) => {
    has = true;
  }
  try { // it doesn't matter if it fails
    flush_NO_FILESYSTEM();
  } catch(e) {}
  out = oldOut;
  err = oldErr;
  if (has) {
    warnOnce('stdio streams had content in them that was not flushed. you should set EXIT_RUNTIME to 1 (see the Emscripten FAQ), or make sure to emit a newline when you printf etc.');
    warnOnce('(this may also be due to not including full filesystem support - try building with -sFORCE_FILESYSTEM)');
  }
}

var wasmExports;

// With async instantation wasmExports is assigned asynchronously when the
// instance is received.
createWasm();

run();

// end include: postamble.js

