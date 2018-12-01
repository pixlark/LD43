extern crate sdl2;

use sdl2::image;
use sdl2::image::LoadTexture;

use sdl2::event::Event;
use sdl2::pixels::Color;
use sdl2::rect::Rect;
use sdl2::render::{Texture, TextureCreator};

const WINDOW_SIZE: (usize, usize) = (800, 600);

type WindowCanvas = sdl2::render::Canvas<sdl2::video::Window>;

struct SDLState {
    canvas: WindowCanvas,
    creator: TextureCreator<sdl2::video::WindowContext>,
}

trait GameState {
    fn process_event(&mut self, event: Event);
    fn update(&mut self);
    fn render(&mut self, sdl_state: &mut SDLState);
}

#[derive(Copy, Clone, Debug)]
struct v2 {
    x: i32,
    y: i32,
}

impl v2 {
    fn new(x: i32, y: i32) -> v2 {
        v2 { x: x, y: y }
    }
}

#[derive(Debug)]
enum Ingredient {
    Goldfish,
    Pheasant,
    Turkey,
    Lamb,
    Goat,
    Isaac,
}

#[derive(Debug)]
struct ActiveIngredient {
    ingredient: Ingredient,
    pos: v2,
}

mod UIConstants {
    const INGREDIENT_SIZE: usize = 98;
}

struct StatePlaying<'a> {
    bg_image: Texture<'a>,
    active_ingredients: Vec<ActiveIngredient>,
}

impl<'a> StatePlaying<'a> {
    fn new(sdl_state: &'a mut SDLState) -> StatePlaying<'a> {
        StatePlaying {
            bg_image: sdl_state.creator.load_texture("../resources/bg.png").unwrap(),
            active_ingredients: Vec::new(),
        }
    }
}

impl<'a> GameState for StatePlaying<'a> {
    fn process_event(&mut self, event: Event) {
        match event {
            Event::MouseButtonDown { .. } => {
                self.active_ingredients.push(ActiveIngredient {
                    ingredient: Ingredient::Goat,
                    pos: v2::new(10, 10),
                });
            }
            _ => (),
        }
    }
    fn update(&mut self) {}
    fn render(&mut self, sdl_state: &mut SDLState) {
        sdl_state.canvas.set_draw_color(Color::RGB(0x80, 0x80, 0x80));
        // Render active ingredients
        for ai in self.active_ingredients.iter() {
            sdl_state.canvas
                .fill_rect(Rect::new(ai.pos.x, ai.pos.y, 50, 50))
                .unwrap();
        }
    }
}

trait Stack<T> {
    fn top(&mut self) -> Option<&mut T>;
}

impl<T> Stack<T> for Vec<T> {
    fn top(&mut self) -> Option<&mut T> {
        if self.is_empty() {
            None
        } else {
            let last = self.len() - 1;
            Some(&mut self[last])
        }
    }
}

type GameStateStack = Vec<Box<dyn GameState>>;

fn main() {
    let sdl_context = sdl2::init().unwrap();
    let sdl_image_context = image::init(image::InitFlag::all()).unwrap();

    let mut game_state_stack = GameStateStack::new();

    let mut sdl_state = {
        let video_system = sdl_context.video().unwrap();
        let window = video_system
            .window("LD43", WINDOW_SIZE.0 as u32, WINDOW_SIZE.1 as u32)
            .position_centered()
            .build()
            .unwrap();
        let mut canvas = window.into_canvas().build().unwrap();
        let mut creator = canvas.texture_creator();
        SDLState { canvas: canvas, creator: creator }
    };

    game_state_stack.push(Box::new(StatePlaying::new(&mut sdl_state)));

    let mut event_pump = sdl_context.event_pump().unwrap();

    let mut force_quit = false;
    while !game_state_stack.is_empty() && !force_quit {
        let state = game_state_stack.top().unwrap();
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit { .. } => force_quit = true,
                _ => state.process_event(event),
            }
        }
        state.update();
        sdl_state.canvas.set_draw_color(Color::RGB(0xBB, 0xAD, 0xA0));
        sdl_state.canvas
            .fill_rect(Rect::new(0, 0, WINDOW_SIZE.0 as u32, WINDOW_SIZE.1 as u32))
            .unwrap();
        state.render(&mut sdl_state);
        sdl_state.canvas.present();
    }
}
