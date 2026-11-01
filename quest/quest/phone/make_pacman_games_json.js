// Fixed one-game metadata for the native Cardboard phone build. The general
// Quest generator is PowerShell-based; this keeps the phone build independent
// of host PowerShell language-mode policies.
const fs = require('fs');
const path = require('path');

const output = process.argv[2];
if (!output) throw new Error('Usage: node make_pacman_games_json.js <output.json>');

const env = {
    VPIX_PLAY: '1',
    VPIX_LOG: '@LOGS@/pac',
    VPIX_HOOKS: '@ENGINE@/hooks/PAC.hooks',
    VPIX_SETTINGS: '@SETTINGS@/vpix_settings.txt',
    VPIX_CDSIG: '@CDSIG@',
    VPIX_FPS: '0',
    VPIX_AIM_HEAD: '0',
    VPIX_NOWALK: '0',
    VPIX_WALK_TURN: '1',
    VPIX_TRIG_WALK: '1',
    VPIX_HUD_SCALE: '0.8',
    VPIX_NOTURN: '0',
    VPIX_SCREEN_CLIP: '0',
    VPIX_SCREEN_H: '0',
    VPIX_FLIC_PACE: '0',
    VPIX_GLYPH_COLOUR: '0',
    VPIX_SCARED_FACE: '1',
    VPIX_FLIC_CENTRE: '1',
    VPIX_MATERIALS: '1',
    VPIX_HIDE_OWN_HEAD: '0',
    VPIX_CROUCH: '0',
    VPIX_HOLD_VIEW: '0',
    VPIX_HAPTIC: 'death.wav=b:400:1',
    VPIX_ARM_TO_GUN: '0',
    VPIX_TIMER_COPY: '0',
    VPIX_NEAR_MODELS: '',
    VPIX_ONE_SIDED: '',
    VPIX_POLY_SELECT: '',
    VPIX_MUZZLE: '',
    VPIX_PICTURE_HEADLOCK: '0',
    VPIX_PICTURE_WAIT_MS: '0',
    VPIX_PANEL_WORLD: '0',
    VPIX_FREE_REF_LATCH: '0',
    VPIX_FREE_YAW_MAX_DEG: '0',
    VPIX_FULLPIC_LOCK: '0',
    VPIX_SUB_LARGEST_FIRST: '0',
    VPIX_FOLLOW_YAW: '0',
    VPIX_CLAMP_Y: '',
    VPIX_MOVE_Y: '',
    VPIX_FOLLOW_LATCH: '0',
    VPIX_FOLLOW_PITCH: '0',
    VPIX_STICK_TWIST: '0',
    VPIX_WORLD_SCALE: '',
    VPIX_BLIT_NUDGE: '',
    VPIX_ON_TOP: '',
    VPIX_PICTURE_BACK: '0',
    VPIX_BLIT_PRIORITY: '0',
    VPIX_BLIT_UNDER: 'bells',
    VPIX_HIDE_ABOVE: '0',
    VPIX_DEPTH_FIX: '1',
    VPIX_PRIO_LAYERS: '0',
    VPIX_LIGHT_WORLD: '0',
    VPIX_HIDE_DEATH_BOX: '0',
    VPIX_JOYAXES: '0',
    VPIX_TOP_BUTTONS: '0',
    VPIX_DESK_CONVERGE: '8000',
    VPIX_HAND_ROLL: '0',
    VPIX_LEAN: '0',
    VPIX_HEAD_CENTRE: '0',
    VPIX_BOX_FORCEGLOVE: '0',
    VPIX_BOX_GLOVE_TRACK: '0',
    VPIX_MENU_FWD_FLIP: '0',
    VPIX_BOX_RING_FEED: '0',
    VPIX_DESK_BOX: '0',
    VPIX_TRIM_LOCK: '0',
    VPIX_BOX_KO_LIFT: '0',
    VPIX_DECAL_STACK: '0',
    VPIX_NO_AIM_ASSIST: '0',
    VPIX_AIM_ASSIST_DEG: '0',
    VPIX_AIM_ASSIST_FAR_DEG: '0',
    VPIX_DESK_DOT: '0',
    VPIX_THUMB_ON_B: '0',
    VPIX_GUN_NUDGE: '0',
    VPIX_AIM_MODES: '0',
    VPIX_START_BITS: '0',
    VPIX_BLIT3K_PRIO: '0',
    VPIX_LTRIG_THUMB: '0',
    VPIX_FOG3K: '0',
    VPIX_MULTIVIEW3K: '0',
    VPIX_DESK_ATTRACT_LOCK: '0'
};

const document = {
    made: 'MallVR Cardboard Pac-Man',
    games: [{
        code: 'PAC',
        name: 'Pac-Man VR',
        exe: 'PAC',
        args: '/vcc /solo',
        sha1: '8122b1e2c672363f6b0798d8af0a4b57e715199b',
        cd: 'PACMAN.CUE',
        cdBin: 'PACMAN.BIN',
        env
    }]
};

fs.mkdirSync(path.dirname(path.resolve(output)), { recursive: true });
fs.writeFileSync(output, `${JSON.stringify(document, null, 2)}\n`, 'utf8');
console.log(`Cardboard games.json: 1 game -> ${output}`);
