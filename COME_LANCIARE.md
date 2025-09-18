# Come Compilare e Lanciare So Long Runner

## Test Veloce (Solo Terminal)

Per testare subito le meccaniche del gioco senza MLX:

```bash
# Compila il test semplice
gcc simple_test.c -o simple_test

# Lancia il test
./simple_test
```

**Controlli del test:**
- `WASD` = Muovi il player
- `Q` = Esci
- Il player si muove automaticamente verso l'alto ogni 1.5 secondi
- Il muro sale dal basso ogni 3 secondi
- Raccogli le porte `C` per modificare la tua armata
- Raggiungi `E` dopo aver preso tutte le `C`

## Versione Completa con MLX42

### 1. Installa MLX42 (già fatto)
```bash
# MLX42 è già clonato e compilato nella cartella MLX42/
```

### 2. Compila il gioco completo
```bash
# Compila con MLX42
make
```

### 3. Lancia il gioco
```bash
# Con la mappa di esempio
./so_long_runner example_map.ber

# Oppure test rapido
make test
```

## Meccaniche del Gioco

### Auto-Step Verticale
- Il player sale automaticamente verso l'alto ogni 500ms
- Usa `A`/`D` per spostarti lateralmente e scegliere le corsie
- Movimento manuale possibile con `WASD`

### Sistema dell'Armata
- Inizi con 30 soldatini
- I soldatini ruotano attorno al player
- Più soldatini = più proiettili quando spari (`SPACE`)

### Porte Matematiche (`C`)
Ogni porta modifica la tua armata:
- **+20**: Aggiungi soldatini
- **×2**: Raddoppia l'armata
- **-10**: Riduci l'armata (pericoloso!)

### Nemici
- **Rossi**: Patrol orizzontali
- **Arancioni**: Ti inseguono se sei vicino
- **Viola**: Barre che scendono dall'alto

### Muro che Sale
- Dal basso sale un muro ogni 1.2 secondi
- Se ti raggiunge → Game Over
- Crea pressione costante per salire

### Come Vincere
1. Raccogli **tutte** le porte `C`
2. Raggiungi l'uscita `E` (si sblocca dopo aver preso tutte le C)
3. Non farti raggiungere dal muro
4. Non perdere tutta la tua armata

### Controlli
- **W/A/S/D**: Movimento
- **SPACE**: Spara proiettili
- **ESC**: Esci

## Debugging

Se il gioco non compila:

```bash
# Ricompila MLX42
cd MLX42
cmake --build build
cd ..

# Pulisci e ricompila
make fclean
make
```

## Conclusioni sul Gameplay

Il concetto funziona molto bene! Le meccaniche si completano:
- La pressione del muro crea urgenza
- Le porte matematiche aggiungono decisioni strategiche
- Il sistema dell'armata crea progressione
- I nemici aggiungono ostacoli tattici

È perfetto per un progetto so_long esteso!