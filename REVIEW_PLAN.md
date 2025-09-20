# SO_LONG - PLAN DI FIX (Review ChatGPT)

## STATUS: ANALISI COMPLETATA
**Priority**: MANDATORY compliance PRIMA di qualsiasi bonus

---

## 🚨 CRITICAL BLOCKERS (MANDATORY)

### 1. MAP VALIDATION - MANCANTE COMPLETAMENTE
**Priority: HIGHEST** - Senza questo l'audit fallisce sicuramente

#### 1.1 Estensione .ber
- [ ] Verificare che argv[1] finisca con ".ber"
- [ ] Rifiutare file senza estensione corretta

#### 1.2 Format validation
- [ ] **Rettangolarità**: tutte le righe STESSA lunghezza
- [ ] **Bordi chiusi**: prima/ultima riga + prima/ultima colonna = tutti '1'
- [ ] **Charset validation**: solo {0,1,C,E,P}, rifiutare qualsiasi altro char

#### 1.3 Element counting
- [ ] **Esattamente 1 'P'** (ora prendiamo la prima e non controlliamo)
- [ ] **≥1 'C'** (ora contiamo ma non rifiutiamo se 0)
- [ ] **Esattamente 1 'E'** (ora non contiamo neanche)

#### 1.4 Path validation (FLOOD FILL)
- [ ] Implementare flood-fill da posizione P
- [ ] Verificare che TUTTI i C siano raggiungibili
- [ ] Verificare che E sia raggiungibile
- [ ] Rifiutare mappa se qualcosa non raggiungibile

### 2. MEMORY LEAKS - MLX RESOURCES
**Priority: HIGH** - Valgrind fallisce sicuramente

#### 2.1 Sprite cleanup mancante
- [ ] mlx_destroy_image per OGNI sprite in close_game
- [ ] destroy_assets() function per cleanup ordinato

#### 2.2 MLX cleanup mancante
- [ ] mlx_destroy_display(game->mlx)
- [ ] free(game->mlx)
- [ ] Cleanup su TUTTI i rami di errore

### 3. BUFFER OVERFLOW RISK
**Priority: HIGH**

#### 3.1 Fixed buffer 4000B
- [ ] Sostituire con get_next_line o realloc dinamico
- [ ] File >4000B attualmente vengono troncati SILENZIOSAMENTE

---

## ⚠️ COMPLIANCE ISSUES (MEDIUM)

### 4. ERROR HANDLING
- [ ] Stampare errori su stderr invece di stdout
- [ ] Creare fatal() function per error handling pulito
- [ ] Sostituire printf di errore con write(2, ...)

### 5. MAKEFILE
- [ ] Aggiungere target 'bonus' (richiesto dal subject)
- [ ] Rimuovere -g dai flag finali
- [ ] Struttura per modularizzazione futura

---

## 🐛 MINOR BUGS

### 6. Coordinate safety
- [ ] mlx_string_put con y negativi (y-18, y-10)
- [ ] Clamp coordinates >= 0

### 7. Parser edge cases
- [ ] Multiple P handling (ora prende l'ultimo)
- [ ] Empty lines handling
- [ ] Windows line endings (\r\n)

---

## 📋 IMPLEMENTATION PLAN

### PHASE 1: MAP VALIDATION (CRITICAL)
1. **File extension check**
2. **Robust file reading** (GNL or dynamic)
3. **Rectangle validation**
4. **Wall borders validation**
5. **Charset validation**
6. **Element counting**
7. **Flood fill implementation**

### PHASE 2: MEMORY MANAGEMENT
1. **destroy_assets() function**
2. **Complete close_game() cleanup**
3. **Error path cleanup**

### PHASE 3: ERROR HANDLING
1. **stderr for errors**
2. **fatal() function**
3. **Clean error messages**

### PHASE 4: MAKEFILE & STRUCTURE
1. **bonus target**
2. **Clean flags**
3. **Header preparation**

---

## 🎯 VALIDATION CHECKLIST

### Map Requirements (42 Subject)
- [ ] .ber extension
- [ ] Rectangular (all rows same length)
- [ ] Surrounded by walls (borders all '1')
- [ ] Valid charset only: 0,1,C,E,P
- [ ] Exactly 1 player (P)
- [ ] At least 1 collectible (C)
- [ ] Exactly 1 exit (E)
- [ ] Valid path P->all C->E (flood fill)

### Game Requirements
- [ ] WASD + arrow keys movement
- [ ] ESC closes game
- [ ] X button closes game
- [ ] Move counter on stdout
- [ ] Exit only after all collectibles
- [ ] No crashes, no leaks

### Makefile Requirements
- [ ] all, clean, fclean, re targets
- [ ] bonus target
- [ ] No relinking
- [ ] Proper flags

---

## 🔥 IMMEDIATE ACTION ITEMS

**TODAY:**
1. Implement .ber extension check
2. Add basic rectangle validation
3. Add element counting with proper validation

**NEXT:**
1. Implement flood fill
2. Fix MLX memory leaks
3. Test with valgrind

**FINAL:**
1. Error handling cleanup
2. Makefile bonus target
3. Code review for norminette prep

---

## ⚗️ TEST CASES TO CREATE

### Invalid Maps
- no_extension.txt
- wrong_extension.xyz
- not_rectangle.ber
- no_walls.ber
- invalid_chars.ber
- no_player.ber
- multiple_players.ber
- no_collectibles.ber
- no_exit.ber
- multiple_exits.ber
- unreachable_collectible.ber
- unreachable_exit.ber

### Edge Cases
- single_line.ber
- huge_file.ber
- empty_file.ber
- only_newlines.ber

---

## 💭 NOTES

**ChatGPT Review Assessment: ACCURATE**
- Spotted all major mandatory compliance failures
- Correctly identified memory leaks
- Proper prioritization of fixes
- Good understanding of 42 School requirements

**Current Bonus Status: GOOD**
- Enemies ✅
- Animations ✅
- Move counter in window ✅
- BUT: Bonus only counts if mandatory is PERFECT

**Risk Level: HIGH**
- Multiple blocking issues for audit
- Memory leaks = automatic fail
- Missing validations = automatic fail
- BUT: All fixable with systematic approach

**Estimated Work: 2-3 days**
- Day 1: Map validation + flood fill
- Day 2: Memory management + error handling
- Day 3: Testing + polish