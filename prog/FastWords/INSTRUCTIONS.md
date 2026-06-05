# FastWords — Type Fast, Think Faster!

## Description
A typing speed game where words appear on screen and you must type them before time runs out. Time depends on word length — 4 seconds for short words, up to 10 seconds for long ones.

## Dependencies
- SDL2
- Cairo
- g++ with C++11 support

## Build
```bash
cd prog/FastWords
make
```

Run:
```bash
./FastWords
```

## How to Play
1. A word appears on screen
2. Type it exactly as shown (lowercase)
3. Press **Enter** to submit
4. You have 4–10 seconds depending on word length
5. 3 wrong answers or timeouts = Game Over

## Scoring
- Base points = word length × 5
- Time bonus = remaining seconds × 10
- Streak multiplier — consecutive correct answers increase your streak

## Controls
| Key | Action |
|-----|--------|
| Type | Enter letters |
| Enter | Submit word |
| Escape | Quit game |
| Space/Enter (on game over) | Play again |

## Tips
- Don't rush — accuracy is better than speed
- Long words give more points but less time per letter
- Watch the timer bar — it turns red as time runs out
