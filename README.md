# So Long Runner - Mathematical Army Game

Un gioco runner verticale basato su so_long con meccaniche matematiche innovative.

## Concetto del Gioco

Il player parte dal basso della mappa e deve raggiungere l'uscita 'E' in alto, mentre:
- Un muro sale dal basso applicando pressione costante
- Porte matematiche ('C') modificano la dimensione della tua armata
- Nemici ostacolano il percorso e devono essere abbattuti
- L'armata di soldatini ti segue e genera proiettili

## Meccaniche Principali

### Auto-Step Verticale
- Il player si muove automaticamente verso l'alto ogni 500ms
- Puoi muoverti lateralmente con A/D per scegliere le corsie
- Movimento manuale con WASD (ma l'auto-step continua)

### Sistema dell'Armata
- Inizi con 30 soldatini che ruotano attorno al player
- Più soldatini = più proiettili per volée
- Se l'armata viene completamente distrutta → Game Over

### Porte Matematiche (C)
Ogni porta ha un'operazione diversa:
- **+X**: Aggiungi soldatini all'armata
- **×Y**: Moltiplica l'armata (attenzione ai moltiplicatori!)
- **-Z**: Riduci l'armata (può essere pericoloso)

### Sistema di Combattimento
- **SPACE**: Spara proiettili verso l'alto
- Numero di colpi basato sulla dimensione dell'armata
- Spread maggiore con più soldatini

### Nemici
- **Rossi (Patrol)**: Si muovono avanti/indietro orizzontalmente
- **Arancioni (Chaser)**: Ti inseguono lentamente se sei vicino
- **Viola (Barre)**: Scendono dall'alto per bloccare corsie

### Muro che Sale
- Dal basso, una riga alla volta diventa muro ogni 1.2 secondi
- Se il muro ti raggiunge → Game Over
- Crea pressione costante per salire velocemente

## Controlli

- **W/A/S/D**: Movimento (con auto-step attivo)
- **SPACE**: Spara proiettili
- **ESC**: Esci dal gioco

## Come Vincere

1. Raccogli **tutte** le porte matematiche ('C')
2. Raggiungi l'uscita 'E' (si sblocca dopo aver preso tutte le C)
3. Non farti raggiungere dal muro
4. Non perdere tutta la tua armata

## Come Perdere

- Il muro che sale ti raggiunge
- La tua armata viene completamente distrutta
- Un nemico ti colpisce senza armata

## Compilazione

```bash
make
```

## Esecuzione

```bash
./so_long_runner example_map.ber
```

## Strategia

- **Bilanciamento**: Non sempre prendere tutti i '+', a volte i '-' sono inevitabili
- **Timing**: Gestisci il timing tra raccolta porte e movimento verso l'alto
- **Combattimento**: Usa i proiettili strategicamente per aprire la strada
- **Positioning**: Scegli le corsie giuste per evitare nemici concentrati

## Caratteristiche Tecniche

- Compatibile con i requisiti base di so_long (mappe .ber, raccolta C, uscita E)
- Aggiunge meccaniche avanzate mantenendo la compatibilità
- Sistema modulare facilmente estendibile
- Performance ottimizzate per molti proiettili e nemici

Questo è un prototipo per testare se il gameplay ha senso prima di un'implementazione completa!