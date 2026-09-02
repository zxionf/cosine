// Cosine：视频编辑器 UI 原型
mod data;
mod dsl;

use data::{TrackKind, VideoProject};
use eframe::egui;
use std::path::PathBuf;

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1024.0, 720.0]),
        ..Default::default()
    };

    eframe::run_native(
        "Cosine — Video Editor",
        options,
        Box::new(|_cc| Ok(Box::new(CosineApp::default()))),
    )
}

struct CosineApp {
    /// DSL 代码编辑区内容
    dsl_code: String,
    /// 当前打开的文件路径
    file_path: Option<PathBuf>,
    /// 解析后的项目
    project: Option<VideoProject>,
    /// 输出日志
    output: Vec<String>,
    /// 项目浏览器中展开的时间线
    expanded_timelines: std::collections::HashSet<usize>,
}

impl Default for CosineApp {
    fn default() -> Self {
        Self {
            dsl_code: SAMPLE_DSL.trim().to_string(),
            file_path: None,
            project: None,
            output: vec![String::from("[INFO] Cosine 已启动。编辑 DSL 代码后点击 Edit → Parse 解析。")],
            expanded_timelines: std::collections::HashSet::new(),
        }
    }
}

impl eframe::App for CosineApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // ── 顶部菜单栏 ──
        egui::TopBottomPanel::top("menu_bar").show(ctx, |ui| {
            #[allow(deprecated)]
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open").clicked() {
                        self.open_file();
                        ui.close_menu();
                    }
                    if ui.button("Save").clicked() {
                        self.save_file();
                        ui.close_menu();
                    }
                    ui.separator();
                    if ui.button("Exit").clicked() {
                        std::process::exit(0);
                    }
                });
                ui.menu_button("Edit", |ui| {
                    if ui.button("Parse").clicked() {
                        self.parse_dsl();
                        ui.close_menu();
                    }
                });
                ui.menu_button("Help", |ui| {
                    if ui.button("About").clicked() {
                        self.output.push("[INFO] Cosine v0.1.0 — Rust/egui 视频编辑器原型".into());
                        ui.close_menu();
                    }
                });
            });
        });

        // ── 左侧：项目浏览器 ──
        egui::SidePanel::left("project_browser")
            .resizable(true)
            .default_width(240.0)
            .show(ctx, |ui| {
                ui.heading("Project Browser");
                ui.separator();

                if let Some(ref proj) = self.project {
                    project_tree(ui, proj, &mut self.expanded_timelines);
                } else {
                    ui.label("No project loaded.");
                    ui.label("Open a .cos DSL file and hit Parse.");
                }
            });

        // ── 中央：DSL 代码编辑器 ──
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("DSL Editor");
            if let Some(ref path) = self.file_path {
                ui.label(format!("File: {}", path.display()));
            } else {
                ui.label("File: <unsaved>");
            }
            ui.separator();

            // 代码编辑区域
            egui::ScrollArea::vertical()
                .auto_shrink([false, false])
                .show(ui, |ui| {
                    ui.add_sized(
                        ui.available_size(),
                        egui::TextEdit::multiline(&mut self.dsl_code)
                            .font(egui::TextStyle::Monospace)
                            .code_editor()
                            .desired_rows(30)
                            .lock_focus(true),
                    );
                });
        });

        // ── 底部：输出面板 ──
        egui::TopBottomPanel::bottom("output_log")
            .resizable(true)
            .default_height(150.0)
            .show(ctx, |ui| {
                ui.heading("Output");
                ui.separator();

                egui::ScrollArea::vertical()
                    .auto_shrink([false, false])
                    .stick_to_bottom(true)
                    .show(ui, |ui| {
                        for line in &self.output {
                            ui.label(line);
                        }
                    });
            });
    }
}

// ── 项目浏览器树 ──

fn project_tree(
    ui: &mut egui::Ui,
    proj: &VideoProject,
    expanded: &mut std::collections::HashSet<usize>,
) {
    ui.label(format!("Project: {}", proj.name));

    for (ti, timeline) in proj.timelines.iter().enumerate() {
        let is_open = expanded.contains(&ti);
        let response = ui.selectable_label(is_open, format!("📁 Timeline: {}", timeline.name));

        if response.clicked() {
            if is_open {
                expanded.remove(&ti);
            } else {
                expanded.insert(ti);
            }
        }

        if is_open {
            for (tki, track) in timeline.tracks.iter().enumerate() {
                let icon = match track.kind {
                    TrackKind::Video => "🎬",
                    TrackKind::Audio => "🔊",
                };
                ui.indent(format!("track_{}_{}", ti, tki), |ui| {
                    ui.label(format!("{} Track: {}", icon, track.name));
                    for clip in &track.clips {
                        ui.indent(format!("clip_{}_{}_{}", ti, tki, clip.source), |ui| {
                            ui.label(format!(
                                "🎞 {}  [{:.1}s - {:.1}s]  @{:.1}s",
                                clip.source,
                                clip.src_start,
                                clip.src_end,
                                clip.timeline_in
                            ));
                        });
                    }
                });
            }
        }
    }
}

// ── 文件操作 ──

impl CosineApp {
    fn open_file(&mut self) {
        if let Some(path) = rfd::FileDialog::new()
            .add_filter("Cosine DSL", &["cos", "txt"])
            .pick_file()
        {
            match std::fs::read_to_string(&path) {
                Ok(content) => {
                    self.dsl_code = content;
                    self.file_path = Some(path.clone());
                    self.output
                        .push(format!("[INFO] 已打开: {}", path.display()));
                    // 打开后自动解析
                    self.parse_dsl();
                }
                Err(e) => {
                    self.output
                        .push(format!("[ERROR] 无法读取文件: {e}"));
                }
            }
        }
    }

    fn save_file(&mut self) {
        let path = match &self.file_path {
            Some(p) => p.clone(),
            None => {
                // 没有路径时弹出保存对话框
                match rfd::FileDialog::new()
                    .add_filter("Cosine DSL", &["cos"])
                    .save_file()
                {
                    Some(p) => p,
                    None => return,
                }
            }
        };

        match std::fs::write(&path, &self.dsl_code) {
            Ok(_) => {
                self.file_path = Some(path.clone());
                self.output
                    .push(format!("[INFO] 已保存: {}", path.display()));
            }
            Err(e) => {
                self.output
                    .push(format!("[ERROR] 无法保存: {e}"));
            }
        }
    }

    /// 解析 DSL（使用 logos 词法分析 + 递归下降解析器）
    fn parse_dsl(&mut self) {
        self.output.push("[INFO] 开始解析 DSL...".into());

        match dsl::parser::Parser::parse(&self.dsl_code) {
            Ok(project) => {
                let clip_count: usize = project
                    .timelines
                    .iter()
                    .flat_map(|t| t.tracks.iter())
                    .map(|t| t.clips.len())
                    .sum();
                self.output.push(format!(
                    "[INFO] 解析成功 — {} 个时间线, {} 个片段",
                    project.timelines.len(),
                    clip_count
                ));
                self.project = Some(project);
            }
            Err(e) => {
                self.output.push(format!("[ERROR] 解析失败: {e}"));
                self.project = None;
            }
        }
    }
}

// ── 示例 DSL 脚本 ──

const SAMPLE_DSL: &str = r#"
// 示例 Cosine DSL 脚本
project "My Video" {
    timeline "Main" {
        track "Video 1" video {
            clip "intro.mp4" 0.0..5.0 @ 0.0
            clip "main.mp4" 2.0..12.0 @ 5.0
        }
        track "Audio 1" audio {
            clip "bgm.wav" 0.0..15.0 @ 0.0
        }
    }
}
"#;
