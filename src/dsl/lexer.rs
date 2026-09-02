// DSL 词法分析器：基于 logos
use logos::Logos;

/// 词法 Token
#[derive(Logos, Debug, PartialEq, Clone)]
#[logos(skip r"[ \t\n\r]+")]
pub enum Token {
    // 关键字
    #[token("project")]
    Project,
    #[token("timeline")]
    Timeline,
    #[token("track")]
    Track,
    #[token("clip")]
    Clip,
    #[token("video")]
    Video,
    #[token("audio")]
    Audio,

    // 分隔符
    #[token("{")]
    LBrace,
    #[token("}")]
    RBrace,

    // 操作符
    #[token("..")]
    RangeOp,
    #[token("@")]
    At,

    // 字面量
    #[regex(r#""[^"]*""#, |lex| lex.slice()[1..lex.slice().len()-1].to_string())]
    StringLit(String),

    #[regex(r"[0-9]+\.[0-9]+|[0-9]+", |lex| lex.slice().parse::<f64>().unwrap())]
    Number(f64),

    // 单行注释（跳过）
    #[regex(r"//[^\n]*", logos::skip, allow_greedy = true)]
    Comment,
}
