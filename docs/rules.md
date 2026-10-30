# Rules

A rule is one card. It reads like a sentence:

<span class="card"><b class="w">WHEN</b> trigger <b class="i">IF</b> conditions (optional) <b class="t">THEN</b> actions</span>

- Every rule whose trigger and conditions match runs. They run from top to bottom.
- A card has up to 2 conditions. All of them must hold.
- A card has 1 to 6 actions. They run in order.

There are **5 triggers, 2 conditions and 11 actions**, and nothing else.

!!! tip "Every new game starts with one rule"
    <span class="card"><b class="w">WHEN</b> you press an arrow <b class="t">THEN</b> you move that way</span>
    Delete it to make a one-button game. Change it to "you turn that way" for a
    tank game.

## Triggers

| Trigger | Card text | Runs |
|---------|-----------|------|
| Start | When the screen starts | Once, each time you enter the screen. Also when the game starts. |
| Press | When you press A / B / an arrow / up / down / left / right | When the button goes down. |
| Bump | When X bumps Y | When an X moves into a cell that holds a Y. X and Y can be a thing, "you" or "anything". |
| Beat | Every N beats, X… | Every N beats, once for each X on the screen. |
| Counter | When Counter reaches N | Once, when the counter changes to N. |

## Conditions

| Condition | Card text | Holds when |
|-----------|-----------|------------|
| Chance | 1 in N chance | A random roll comes up. With N = 4 it holds about one time in four. |
| Counter | Counter is at least / at most / exactly N | The counter's value matches. |

## Actions

| Action | Card text | What happens |
|--------|-----------|--------------|
| Move | …moves *way* | The thing tries to move one cell. If the cell is full, it bumps what is there. A move can cause another bump, so pushes chain. |
| Turn | …turns *way* | The thing faces a new way without moving. This changes what "forward" means. |
| Become | …becomes *Kind* | The thing changes kind. It keeps its place and the way it faces. |
| Vanish | …vanishes | The thing is removed. Its cell is free. |
| Make | …makes *Kind* to its *way* | A new thing appears in the next cell that way, only if that cell is empty. It faces that way. |
| Count | Counter goes up / down N, or is set to N | Changes a counter. Counters stay between 0 and 999. |
| Say | Say "text" | A text box opens. The game pauses until you press A. |
| Sound | Play *Blip* | Plays a sound effect. |
| Go | Go to *Screen* | Moves you to another screen. |
| Shake | Shake the screen | The screen shakes for one beat. |
| Win / Lose | You win / You lose "text" | Shows the text. Pressing A starts the game again. |

### Who an action changes

Move, turn, become, vanish and make change a thing. Which one depends on the card:

- In a **bump** card: the *bumper* (the thing that moved) or the *bumped* (the thing that was in the cell). The bumper is the default.
- In a **beat** card: the thing the card runs for.
- In **any** card: every *you* thing.

### Ways

| Way | Means |
|-----|-------|
| up, down, left, right | That direction. |
| forward / back | The way the thing faces, or the opposite. A thing faces the way it last moved or turned. New things face down. |
| that way (same) | The arrow you pressed, or the way the bumper moved. |
| toward you / away from you | One step toward or away from the nearest *you* thing. |
| at random | Up, down, left or right. |

## Recipes

Small sets of cards for common ideas. The editor will offer these from a menu.

| Recipe | Cards |
|--------|-------|
| **Pick-up** | <span class="card"><b class="w">WHEN</b> you bump Coin <b class="t">THEN</b> Coin vanishes · Coins up 1</span> |
| **Push** | <span class="card"><b class="w">WHEN</b> you bump Crate <b class="t">THEN</b> Crate moves that way</span> |
| **Chain push** | Push, plus <span class="card"><b class="w">WHEN</b> Crate bumps Crate <b class="t">THEN</b> the bumped Crate moves that way</span> |
| **Door and key** | <span class="card"><b class="w">WHEN</b> you bump Key <b class="t">THEN</b> Key vanishes · Keys up 1</span><span class="card"><b class="w">WHEN</b> you bump Door <b class="i">IF</b> Keys at least 1 <b class="t">THEN</b> Door vanishes · Keys down 1</span> |
| **Chaser** | <span class="card"><b class="w">WHEN</b> every 2 beats, Wasp <b class="t">THEN</b> moves toward you</span> |
| **Wanderer** | <span class="card"><b class="w">WHEN</b> every beat, Cat <b class="i">IF</b> 1 in 3 chance <b class="t">THEN</b> moves at random</span> |
| **Talker** | <span class="card"><b class="w">WHEN</b> you bump Owl <b class="t">THEN</b> say "Hoo!"</span> |
| **Goal** | <span class="card"><b class="w">WHEN</b> you bump Flag <b class="t">THEN</b> you win "Well done!"</span> |
| **Spreader** | <span class="card"><b class="w">WHEN</b> every beat, Fire <b class="i">IF</b> 1 in 4 chance <b class="t">THEN</b> makes Fire at random</span> |
| **Timer** | <span class="card"><b class="w">WHEN</b> every 4 beats, you <b class="t">THEN</b> Time down 1</span><span class="card"><b class="w">WHEN</b> Time reaches 0 <b class="t">THEN</b> you lose "Out of time."</span> |

## Ideas

| Idea | The trick |
|------|-----------|
| Crate puzzle (turns) | When you bump Crate, Crate moves the same way. When Crate bumps Hole, Hole vanishes and Crate becomes Stone. |
| Spreading fire | Every beat, Fire makes Fire at random (1 in 4 chance). Every 6 beats, Fire becomes Ash. |
| Trail game | Every beat, you move forward and make Tail behind you. Every 6 beats, Tail vanishes. Bumping Tail loses. The arrows turn you. |
| Garden toy | Seeds grow into sprouts and then flowers, each step a 1 in 3 chance. There is no ending, and that's fine. |
| Story walk | Many screens, people who say things when bumped, a counter for what you know, and doors that open when it is high enough. |
| One-button dodge | Delete the arrow rule. Pressing A moves you up, and every beat you fall down. Rocks move left. |
| Shape shifter | Pressing B turns Bee into Mole and back. Mole can dig through Dirt, and Bee can cross Water. |

The exact order in which things happen is on [How a beat runs](beat.md).
