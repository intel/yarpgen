|         | ID                | Status      | Type       | Component         | Description                                                                                         |
|---------|-------------------|-------------|------------|-------------------|-----------------------------------------------------------------------------------------------------|
| 1       | 83383             | FIXED       | wrong code | tree-optimization | Wrong code with a bunch of type conversion and ternary operators                                    |
| 2       | 83382             | UNCONFIRMED | wrong code | sanitizer         | UBSAN tiggers false-positive warning [-Werror=uninitialized]                                        |
| 3       | 83252             | FIXED       | wrong code | target            | [8 Regression] Wrong code with "-march=skylake-avx512 -O3"                                          |
| 4       | 83221             | FIXED       | ICE        | tree-optimization | [8 Regression] qsort comparator not anti-commutative: -2147483648, -2147483648                      |
| 5       | 82778             | FIXED       | ICE        | rtl-optimization  | crash: insn does not satisfy its constraints                                                        |
| 6       | 82576             | NEW         | ICE        | rtl-optimization  | sbitmap_vector_alloc() not ready for 64 bits                                                        |
| 7       | 82413             | FIXED       | ICE        | c                 | [8 Regression] -O0 crash (ICE in decompose, at tree.h:5179)                                         |
| 8       | 82381             | FIXED       | ICE        | tree-optimization | [8 Regression] internal compiler error: qsort checking failed                                       |
| 9       | 82353             | FIXED       | wrong code | sanitizer         | [8 Regression] runtime ubsan crash                                                                  |
| 10      | 82192             | FIXED       | wrong code | rtl-optimization  | [5/6/7/8 Regression] gcc produces incorrect code with -O2 and bit-field                             |
| 11      | 81987             | FIXED       | ICE        | tree-optimization | [8 Regression] ICE in verify_ssa with -O3 -march=skylake-avx512                                     |
| 12      | 81814             | FIXED       | wrong code | middle-end        | [5/6/7 Regression] Incorrect behaviour at -O0 (conditional operator)                                |
| 13      | 81705             | FIXED       | wrong code | middle-end        | [8 Regression] UBSAN: yet another false positive                                                    |
| 14      | 81607             | FIXED       | ICE        | c++               | [6 Regression] Conditional operator: "type mismatch in shift expression" error                      |
| 15      | 81588             | FIXED       | wrong code | tree-optimization | [7/8 Regression] Wrong code at -O2                                                                  |
| 16      | 81556             | FIXED       | wrong code | tree-optimization | [7/8 Regression] Wrong code at -O2                                                                  |
| 17      | 81555             | FIXED       | wrong code | tree-optimization | [5/6/7/8 Regression] Wrong code at -O1                                                              |
| 18      | 81553             | FIXED       | ICE        | rtl-optimization  | [7/8 Regression] ICE in immed_wide_int_const, at emit-rtl.c:607                                     |
| 19      | 81546             | FIXED       | ICE        | tree-optimization | [8 Regression] ICE at -O3 during GIMPLE pass dom                                                    |
| 20      | 81503             | FIXED       | wrong code | tree-optimization | [8 Regression] Wrong code at -O2                                                                    |
| 21      | 81488             | FIXED       | wrong code | sanitizer         | [8 Regression] gcc goes off the limits allocating memory in gimple-ssa-strength-reduction.c         |
| 22      | 81423             | FIXED       | wrong code | rtl-optimization  | [6/7/8 Regression] Wrong code at -O2                                                                |
| 23      | 81403             | FIXED       | wrong code | tree-optimization | [8 Regression] wrong code at -O3                                                                    |
| 24      | 81387             | UNCONFIRMED | wrong code | sanitizer         | UBSAN consumes too much memory at -O2                                                               |
| 25      | 81281             | FIXED       | wrong code | sanitizer         | [6/7/8 Regression] UBSAN: false positive, dropped promotion to long type.                           |
| 26      | 81162             | FIXED       | wrong code | tree-optimization | [8 Regression] UBSAN switch triggers incorrect optimization in SLSR                                 |
| 27      | 81148             | FIXED       | wrong code | sanitizer         | UBSAN: two more false positives                                                                     |
| 28      | 81097             | FIXED       | wrong code | sanitizer         | UBSAN: false positive for not existing negation operator and a bogus message                        |
| 29      | 81088             | FIXED       | wrong code | middle-end        | UBSAN: false positive as a result of reassosiation                                                  |
| 30      | 81065             | FIXED       | wrong code | sanitizer         | UBSAN: false positive as a result of distribution involving different types                         |
| 31      | 80932             | FIXED       | wrong code | sanitizer         | UBSAN: false positive as a result of distribution: c1*(c2*v1-c3*v2)=>c1*c2*v1-c1*c3*v2              |
| 32      | 80875             | FIXED       | ICE        | sanitizer         | [7 Regression] UBSAN: compile time crash in fold_binary_loc at fold-const.c:9817                    |
| 33      | 80800             | FIXED       | wrong code | sanitizer         | UBSAN: yet another false positive                                                                   |
| 34      | 80620             | FIXED       | wrong code | tree-optimization | [8 Regression] gcc produces wrong code with -O3                                                     |
| 35      | 80597             | FIXED       | ICE        | ipa               | [8 Regression] internal compiler error: in compute_inline_parameters, at ipa-inline-analysis.c:3126 |
| 36      | 80536             | FIXED       | ICE        | sanitizer         | [6/7/8 Regression] UBSAN: compile time segfault                                                     |
| 37      | 80403             | FIXED       | ICE        | sanitizer         | UBSAN: compile time crash with "type mismatch in binary expression" message in / and % expr         |
| 38      | 80386             | FIXED       | wrong code | sanitizer         | UBSAN: false positive - constant folding and reassosiation before instrumentation                   |
| 39      | 80362             | FIXED       | wrong code | middle-end        | [5/6 Regression] gcc miscompiles arithmetic with signed char                                        |
| 40      | 80350             | FIXED       | wrong code | sanitizer         | UBSAN changes code semantics when -fno-sanitize-recover=undefined is used                           |
| 41      | 80349             | FIXED       | ICE        | sanitizer         | [6/7 Regression] UBSAN: compile time crash with "type mismatch in binary expression" message        |
| 42      | 80348             | FIXED       | ICE        | sanitizer         | [6 Regression] UBSAN: compile time crash in ubsan_instrument_division                               |
| 43      | 80341             | FIXED       | wrong code | middle-end        | [5/6 Regression] gcc miscompiles division of signed char                                            |
| 44      | 80297             | FIXED       | ICE        | c++               | [6 Regression] Compiler time crash: type mismatch in binary expression                              |
| 45      | 80072             | FIXED       | ICE        | tree-optimization | [7 Regression] ICE in gimple_build_assign_1 with -O3 -march=broadwell/skylake-avx512                |
| 46      | 80067             | FIXED       | ICE        | sanitizer         | [6/7 Regression] ICE in fold_comparison with -fsanitize=undefined                                   |
| 47      | 80054             | FIXED       | ICE        | tree-optimization | [7 Regression] ICE in verify_ssa with -O3 -march=broadwell/skylake-avx512                           |
| 48      | 79399             | FIXED       | ICE        | middle-end        | GCC fails to compile big source at -O0                                                              |
| 49      | 78726             | FIXED       | wrong code | middle-end        | [5/6 Regression] Incorrect unsigned arithmetic optimization                                         |
| 50      | 78720             | FIXED       | wrong code | tree-optimization | [7 Regression] Illegal instruction in generated code                                                |
| 51      | 78438             | FIXED       | wrong code | rtl-optimization  | [7 Regression] incorrect comparison optimization                                                    |
| 52      | 78436             | FIXED       | wrong code | tree-optimization | [7 Regression] incorrect write to larger-than-type bitfield (signed char x:9)                       |
| 53      | 78132             | FIXED       | wrong code | rtl-optimization  | [7 Regression] GCC produces invalid instruction (kmovd and kmovq) for KNL.                          |
| 54      | 77544             | FIXED       | ICE        | tree-optimization | [6 Regression] segfault at -O0 - infinite loop in simplification                                    |
| 55      | 77476             | FIXED       | wrong code | target            | [7 Regression] [AVX-512] illegal kmovb instruction on KNL                                           |
| 56      | 73714             | FIXED       | wrong code | tree-optimization | [Regression 7] Incorrect unsigned long long arithmetic optimization                                 |
| 57      | 72835             | FIXED       | wrong code | tree-optimization | [7 Regression] Incorrect arithmetic optimization involving bitfield arguments                       |
| 58      | 71657             | NEW         | wrong code | target            | Wrong code on trunk gcc (std::out_of_range), westmere                                               |
| 59      | 71655             | FIXED       | ICE        | tree-optimization | [7 Regression] GCC trunk ICE on westmere target                                                     |
| 60      | 71488             | FIXED       | wrong code | tree-optimization | [6 Regression] Wrong code for vector comparisons with ivybridge and westmere targets                |
| 61      | 71470             | FIXED       | wrong code | target            | Wrong code on trunk gcc with westmere target                                                        |
| 62      | 71389             | FIXED       | ICE        | target            | [7 Regression] ICE on trunk gcc on ivybridge target (df_refs_verify)                                |
| 63      | 71281             | FIXED       | ICE        | target            | [7 Regression] ICE on gcc trunk on knl, wsm, ivb and bdw targets (tree-ssa-reassoc)                 |
| 64      | 71279             | FIXED       | ICE        | middle-end        | [6/7 Regression] ICE on trunk gcc with knl target                                                   |
| 65      | 71261             | FIXED       | ICE        | tree-optimization | [7 Regression] Trunk GCC hangs on knl and broadwell targets                                         |
| 66      | 71259             | FIXED       | wrong code | tree-optimization | [6/7 Regression] GCC trunk emits wrong code                                                         |
| 67      | 70941             | FIXED       | wrong code | target            | [5 Regression] Test miscompiled with -O2.                                                           |
| 68      | 70902             | SUSPENDED   | ICE        | rtl-optimization  | [7 Regression] GCC freezes while compiling for 'skylake-avx512' target                              |
| 69      | 70728             | FIXED       | wrong code | target            | GCC trunk emits invalid assembly for knl target                                                     |
| 70      | 70726             | FIXED       | ICE        | tree-optimization | [6/7 Regression] Internal compiler error (ICE) on valid code                                        |
| 71      | 70725             | FIXED       | ICE        | tree-optimization | Internal compiler error (ICE) on valid code                                                         |
| 72      | 70542             | FIXED       | wrong code | rtl-optimization  | [6 Regression] Wrong code with -O3 -mavx2.                                                          |
| 73      | 70450             | FIXED       | wrong code | target            | [6 Regression] Wrong code with -O0 and -O1.                                                         |
| 74      | 70429             | FIXED       | wrong code | rtl-optimization  | Wrong code with -O1.                                                                                |
| 75      | 70354             | FIXED       | wrong code | tree-optimization | [6 Regression] Wrong code with -O3 -march=broadwell and -march=skylake-avx512.                      |
| 76      | 70333             | FIXED       | wrong code | target            | [5 Regression] Test miscompiled with -O0.                                                           |
| 77      | 70252             | FIXED       | ICE        | tree-optimization | ICE in vect_get_vec_def_for_stmt_copy with -O3 -march=skylake-avx512.                               |
| 78      | 70251             | FIXED       | wrong code | tree-optimization | Wrong code with -O3 -march=skylake-avx512.                                                          |
| 79      | 70222             | FIXED       | wrong code | rtl-optimization  | Test miscompiled with -O1                                                                           |
| 80      | 70153             | FIXED       | ICE        | c++               | [6 Regression] ICE on valid C++ code                                                                |
| 81      | 70026             | FIXED       | ICE        | tree-optimization | [6 Regression] ICE in expand_expr_real_2 with -O1 -ftree-vectorize                                  |
| 82      | 70021             | FIXED       | wrong code | target            | [6 Regression] Test miscompiled with -O3 option for -march=core-avx2.                               |
| 83      | 69820             | FIXED       | wrong code | tree-optimization | [6 Regression] Test miscompiled with -O3 option                                                     |
|         |                   |             |            |                   |                                                                                                     |
| SUMMARY | Status            |             |            |                   |                                                                                                     |
|         | FIXED             | 78          |            |                   |                                                                                                     |
|         | UNCONFIRMED       | 2           |            |                   |                                                                                                     |
|         | NEW               | 2           |            |                   |                                                                                                     |
|         | SUSPENDED         | 1           |            |                   |                                                                                                     |
|         |                   |             |            |                   |                                                                                                     |
|         | Type              |             |            |                   |                                                                                                     |
|         | wrong code        | 51          |            |                   |                                                                                                     |
|         | ICE               | 32          |            |                   |                                                                                                     |
|         |                   |             |            |                   |                                                                                                     |
|         | Component         |             |            |                   |                                                                                                     |
|         | tree-optimization | 30          |            |                   |                                                                                                     |
|         | sanitizer         | 18          |            |                   |                                                                                                     |
|         | target            | 11          |            |                   |                                                                                                     |
|         | rtl-optimization  | 11          |            |                   |                                                                                                     |
|         | c                 | 1           |            |                   |                                                                                                     |
|         | middle-end        | 8           |            |                   |                                                                                                     |
|         | c++               | 3           |            |                   |                                                                                                     |
|         | ipa               | 1           |            |                   |                                                                                                     |
