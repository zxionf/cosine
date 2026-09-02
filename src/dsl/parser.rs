// DSL 递归下降解析器
use crate::data::{Clip, Timeline, Track, TrackKind, VideoProject};
use logos::Logos;
use super::lexer::Token;

/// 解析错误
#[derive(Debug)]
pub struct ParseError {
    pub message: String,
}

impl std::fmt::Display for ParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Parse error: {}", self.message)
    }
}

/// 解析器：将 token 流转换为 VideoProject
pub struct Parser {
    tokens: std::vec::IntoIter<Token>,
    peeked: Option<Token>,
}

impl Parser {
    /// 解析一段 DSL 源码，返回 VideoProject
    pub fn parse(source: &str) -> Result<VideoProject, ParseError> {
        let lexer = Token::lexer(source);
        let tokens: Vec<Token> = lexer
            .spanned()
            .filter_map(|(tok, _span): (Result<Token, ()>, _)| tok.ok())
            .collect();

        let mut parser = Parser {
            tokens: tokens.into_iter(),
            peeked: None,
        };

        parser.parse_project()
    }

    // ── 辅助方法 ──

    /// 偷看下一个 token，不消费
    fn peek(&mut self) -> Option<&Token> {
        if self.peeked.is_none() {
            self.peeked = self.tokens.next();
        }
        self.peeked.as_ref()
    }

    /// 消费下一个 token
    fn next(&mut self) -> Option<Token> {
        self.peeked.take().or_else(|| self.tokens.next())
    }

    /// 期望下一个 token 为指定类型，否则报错
    fn expect(&mut self, expected: Token) -> Result<Token, ParseError> {
        match self.next() {
            Some(tok) if std::mem::discriminant(&tok) == std::mem::discriminant(&expected) => Ok(tok),
            Some(tok) => Err(ParseError {
                message: format!("expected {:?}, found {:?}", expected, tok),
            }),
            None => Err(ParseError {
                message: format!("expected {:?}, but reached end of input", expected),
            }),
        }
    }

    /// 消费一个字符串 token，返回内部值
    fn expect_string(&mut self) -> Result<String, ParseError> {
        match self.next() {
            Some(Token::StringLit(s)) => Ok(s),
            Some(tok) => Err(ParseError {
                message: format!("expected a string literal, found {:?}", tok),
            }),
            None => Err(ParseError {
                message: "expected a string literal, but reached end of input".into(),
            }),
        }
    }

    /// 消费一个数字 token，返回内部值
    fn expect_number(&mut self) -> Result<f64, ParseError> {
        match self.next() {
            Some(Token::Number(n)) => Ok(n),
            Some(tok) => Err(ParseError {
                message: format!("expected a number, found {:?}", tok),
            }),
            None => Err(ParseError {
                message: "expected a number, but reached end of input".into(),
            }),
        }
    }

    // ── 递归下降解析规则 ──

    /// project "name" { timeline* }
    fn parse_project(&mut self) -> Result<VideoProject, ParseError> {
        self.expect(Token::Project)?;
        let name = self.expect_string()?;
        self.expect(Token::LBrace)?;

        let mut timelines = Vec::new();
        loop {
            match self.peek() {
                Some(Token::Timeline) => {
                    timelines.push(self.parse_timeline()?);
                }
                Some(Token::RBrace) | None => break,
                Some(tok) => {
                    return Err(ParseError {
                        message: format!("unexpected token {:?} in project body", tok),
                    });
                }
            }
        }

        self.expect(Token::RBrace)?;
        Ok(VideoProject { name, timelines })
    }

    /// timeline "name" { track* }
    fn parse_timeline(&mut self) -> Result<Timeline, ParseError> {
        self.expect(Token::Timeline)?;
        let name = self.expect_string()?;
        self.expect(Token::LBrace)?;

        let mut tracks = Vec::new();
        loop {
            match self.peek() {
                Some(Token::Track) => {
                    tracks.push(self.parse_track()?);
                }
                Some(Token::RBrace) | None => break,
                Some(tok) => {
                    return Err(ParseError {
                        message: format!("unexpected token {:?} in timeline body", tok),
                    });
                }
            }
        }

        self.expect(Token::RBrace)?;
        Ok(Timeline { name, tracks })
    }

    /// track "name" (video|audio) { clip* }
    fn parse_track(&mut self) -> Result<Track, ParseError> {
        self.expect(Token::Track)?;
        let name = self.expect_string()?;

        // 轨道的类型：video 或 audio
        let kind = match self.next() {
            Some(Token::Video) => TrackKind::Video,
            Some(Token::Audio) => TrackKind::Audio,
            Some(tok) => {
                return Err(ParseError {
                    message: format!("expected 'video' or 'audio' for track kind, found {:?}", tok),
                });
            }
            None => {
                return Err(ParseError {
                    message: "expected 'video' or 'audio', but reached end of input".into(),
                });
            }
        };

        self.expect(Token::LBrace)?;

        let mut clips = Vec::new();
        loop {
            match self.peek() {
                Some(Token::Clip) => {
                    clips.push(self.parse_clip()?);
                }
                Some(Token::RBrace) | None => break,
                Some(tok) => {
                    return Err(ParseError {
                        message: format!("unexpected token {:?} in track body", tok),
                    });
                }
            }
        }

        self.expect(Token::RBrace)?;
        Ok(Track { name, kind, clips })
    }

    /// clip "source" <start>..<end> @ <position>
    fn parse_clip(&mut self) -> Result<Clip, ParseError> {
        self.expect(Token::Clip)?;
        let source = self.expect_string()?;
        let src_start = self.expect_number()?;
        self.expect(Token::RangeOp)?;
        let src_end = self.expect_number()?;
        self.expect(Token::At)?;
        let timeline_in = self.expect_number()?;

        Ok(Clip {
            source,
            src_start,
            src_end,
            timeline_in,
        })
    }
}
