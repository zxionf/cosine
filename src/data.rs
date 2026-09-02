// 核心数据结构：定义视频编辑项目的所有类型

/// 视频项目（顶层结构）
#[derive(Debug, Clone)]
pub struct VideoProject {
    pub name: String,
    pub timelines: Vec<Timeline>,
}

/// 时间线
#[derive(Debug, Clone)]
pub struct Timeline {
    pub name: String,
    pub tracks: Vec<Track>,
}

/// 轨道（视频轨 / 音频轨）
#[derive(Debug, Clone)]
pub struct Track {
    pub name: String,
    pub kind: TrackKind,
    pub clips: Vec<Clip>,
}

/// 轨道类型
#[derive(Debug, Clone, PartialEq)]
pub enum TrackKind {
    Video,
    Audio,
}

/// 片段
#[derive(Debug, Clone)]
pub struct Clip {
    pub source: String,
    /// 在源素材中的起始时间（秒）
    pub src_start: f64,
    /// 在源素材中的结束时间（秒）
    pub src_end: f64,
    /// 在时间线上的入点（秒）
    pub timeline_in: f64,
}

#[allow(dead_code)]
impl Clip {
    /// 片段时长
    pub fn duration(&self) -> f64 {
        self.src_end - self.src_start
    }

    /// 在时间线上的出点
    pub fn timeline_out(&self) -> f64 {
        self.timeline_in + self.duration()
    }
}
