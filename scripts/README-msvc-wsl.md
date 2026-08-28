# Testing MSVC from WSL

YARPGen can drive Microsoft's `cl.exe` (installed on the Windows host, outside
WSL) as just another compiler in the differential-testing pool, alongside the
Linux gcc/clang. This is done through [`cl-proxy`](cl-proxy), a shim that
presents a GCC/Clang-style command line and forwards to MSVC — the same idea as
[`ispc-proxy`](ispc-proxy).

## Requirements

- **Visual Studio (Build Tools) 2019/2022/2026** installed on the Windows host.
  `cl-proxy` auto-detects `vcvars64.bat` in the common install locations; set
  `YARPGEN_VCVARS` to its full Windows path if detection fails.
- **`scripts/` on your `PATH`** so `make` can find `cl-proxy` (same as
  `ispc-proxy`).
- **Run everything under `/mnt/<drive>/`** (e.g. `/mnt/c/...`). `cl.exe` cannot
  see WSL paths like `/home/...`, and the working directory must be a real
  Windows path. Point `run_gen.py -o` at a `/mnt/c` location.
- C++ only (`--std c++`). C / ISPC / SYCL are not handled by the proxy.

## Running

One time build:
```sh
mkdir build && cd build && cmake .. && make
```

For a 1h run:
```sh
export YARPGEN_HOME=/path/to/yarpgen
export PATH="$YARPGEN_HOME/scripts:$PATH"

mkdir -p /mnt/c/yarpgen-scratch && cd /mnt/c/yarpgen-scratch
run_gen.py --std c++ \
           --config-file "$YARPGEN_HOME/scripts/test_sets_msvc.txt" \
           -o testing --target "gcc msvc" -t 60      # -t is in minutes
```

`run_gen.py` automatically passes `--max-array-dims=3` to the generator whenever
an MSVC target is selected (see [Why the array-dimension
cap](#why-the-array-dimension-cap)).

## Testing sets

MSVC runs off its own config, [`test_sets_msvc.txt`](test_sets_msvc.txt), passed
with `--config-file`; the default `test_sets.txt` has no MSVC entries. You can:

- **Add a set** (or uncomment one of the pre-listed but disabled ones) to widen
  the search. Spell the flags the MSVC way (`/O2 /Qpar`).
- **Comment a set out** to get through more programs. 
The default here ships with 3 active sets (`gcc_o1`, `msvc_no_opt`, `msvc_o2`), 
so each program costs 3 compiles and 3 runs; There are 5 additional sets commented out
(`o1`, `os`, `ot`, `avx2`, `par`) that can be easily added back - increasing that cost proportionally
and shrinking the number of programs a night's run covers.

Every shipped set pins `/std:c++20` explicitly, and you can modify it or add sets
with a different value. Without the explicit switches gcc compiles
C++11 while MSVC compiles C++14.

## Speed: exclude the work folder from Defender

Microsoft Defender's real-time scanning dominates the run time of this setup.
Exclude the scratch directory — elevated PowerShell, on the Windows side:

```powershell
Add-MpPreference -ExclusionPath 'C:\yarpgen-scratch'
Get-MpPreference | Select-Object -ExpandProperty ExclusionPath   # verify
```

Keep this to a directory used for nothing else: files under an excluded path are
no longer scanned at all.

## Why the array-dimension cap

The loop generator produces arrays with up to 7 dimensions of size 10–25, i.e.
multi-gigabyte globals. gcc/clang handle these with `-mcmodel=large`; **MSVC has
no large-memory model and a hard 2 GB image limit**, so the default programs often
won't link. The generator therefore gained `--max-array-dims=<n>` (default `0` =
no explicit cap), and `run_gen.py` sets it to `3` when MSVC is in the pool so the
shared test fits every compiler. Tune the value if you want larger arrays.

## Targeting MSVC's auto-vectorizer

MSVC's auto-vectorizer is a narrow, pattern-matching one (unlike gcc/clang's):
it wants a flat, single-statement loop body with unit stride and no nested
control flow, and yarpgen's default output is far more complex than that, so
it essentially never fires. Pass `--simple-loops={some,all}` to `run_gen.py`
to forward the generator's `--simple-loops` flag, which constrains loops to
that narrow shape:

```sh
run_gen.py --std c++ --config-file "$YARPGEN_HOME/scripts/test_sets_msvc.txt" \
           --simple-loops all -o testing --target "gcc msvc" -t 60
```

- `some` (default policy's own "vectorizable" loops also get the stricter
  simple-loop treatment) or `all` (every loop does). `none` (the default)
  omits the flag entirely, i.e. today's behavior.
- This is a distinct, opt-in mode, not something to leave on for general bug
  hunting: it trades away the operator/control-flow diversity that finds
  non-vectorization bugs (this is how the existing 6 MSVC bugs were found,
  all unrelated to vectorization) in exchange for shapes MSVC's vectorizer
  will actually attempt. Run it as its own separate session.
- Even with `all`, only a majority of the resulting loops actually get
  vectorized (checked via `/Qvec-report:2`) — MSVC still declines some for
  reasons like too little in-loop computation or too few iterations.

## Why `/permissive-`

`cl.exe` keeps alive some language extensions and known bugs to allow for backwards
compatibility. yarpgen's intended usage is to find unknown compiler bugs, so we wish 
to filter those out. To that end, `cl-proxy` hardcodes `/permissive-` on 
every `cl.exe` invocation ([cl-proxy:124](cl-proxy#L124)). This turns off MSVC's 
non-conforming language extensions.

MSVC does enable `/permissive-` implicitly under `/std:c++20` and `/std:c++latest`, 
but *not* under `/std:c++14` / `/std:c++17`, where the default is `/permissive`. 
Passing `/permissive-` explicitly keeps conformance behavior uniform across the 
whole `--std` range.

If you're chasing a suspected MSVC-extension issue and *want* the permissive
front end, drop `/permissive-` from the list in `translate()` — there's no
environment knob for it.

## How cl-proxy works

1. **Flag translation** — only the structural flags the *harness* generates are
   rewritten: `-c`→`/c`, `-o X`→`/Fo:X` (compile) or `/Fe:X.exe` (link),
   `-std=c++NN`→`/std:...`, `-D`/`-I`→`/D`/`/I`. It also drops flags MSVC has no
   use for (`-fPIC`, `-mcmodel=large`, `-march=...`, ...). Every invocation gets
   `/nologo /EHsc /w /permissive-` (see above).

   Optimization and feature flags are **not** translated. Write them the MSVC
   way in the config (`/Od`, `/O1`, `/O2`, `/Os`, `/Ot`, `/arch:AVX2`, ...) and
   they reach `cl.exe` verbatim; an unrecognized GCC-style flag is a hard error
   (exit 2), so a set never silently builds at an optimization level other than
   the one it names. Note that `cl.exe` merely warns (D9002) on an unknown
   `/`-flag, so a typo like `/0d` still gets you a default-optimization build.
2. **Environment caching** — `vcvars64.bat` takes ~25 s, so running it per
   compile is unworkable. On first use `cl-proxy` captures the MSVC environment
   once into `~/.cache/yarpgen_msvc_env.json` (override with
   `YARPGEN_MSVC_ENV_CACHE`), then invokes `cl.exe` directly (~0.5 s/call),
   forwarding `INCLUDE`/`LIB`/`LIBPATH` to the Win32 process via `WSLENV`. Delete
   the cache file to refresh it (e.g. after a VS update). If `cl.exe` can't be
   located it falls back to a slower `cmd.exe` + `vcvars` path.
3. **Executable launcher** — MSVC produces a Windows `.exe`; `cl-proxy` writes a
   small shell launcher (the extension-less name the harness expects) that runs
   the `.exe` and strips CRLF from its output, so its checksum matches the
   LF-terminated output of the other compilers.
