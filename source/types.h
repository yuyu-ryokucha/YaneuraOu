﻿#ifndef _TYPES_H_INCLUDED
#define _TYPES_H_INCLUDED

// --------------------
// release configurations
// --------------------

// コンパイル時の設定などは以下のconfig.hを変更すること。
#include "config.h"

// --------------------
//      include
// --------------------

// あまりたくさんここに書くとコンパイルが遅くなるので書きたくないのだが…。

#include "extra/bitop.h"

#include <iostream>     // iostreamに対する<<使うので仕方ない
#include <string>       // std::string使うので仕方ない
#include <algorithm>    // std::max()を使うので仕方ない
#include <limits>		// std::numeric_limitsを使うので仕方ない
#include <chrono>       // std::chrono
#include <vector>       // std::vector
#include <cassert>      // assert

#if defined(_MSC_VER)
// Disable some silly and noisy warnings from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#endif

/// Predefined macros hell:
///
/// __GNUC__                Compiler is GCC, Clang or ICX
/// __clang__               Compiler is Clang or ICX
/// __INTEL_LLVM_COMPILER   Compiler is ICX
/// _MSC_VER                Compiler is MSVC
/// _WIN32                  Building on Windows (any)
/// _WIN64                  Building on Windows 64 bit

namespace YaneuraOu {

// --------------------
//  型の最小値・最大値
// --------------------

// "windows.h"をincludeしていると、maxがマクロによって置換され、そのためstd::max()がコンパイルエラーとなるので、
// それを回避するために(std::max)() と書くテクニックがある。以下で、(std::...::max)() のように書いてあるのはそのため。

constexpr int     int_max   = (std::numeric_limits<int>    ::max)();
constexpr int     int_min   = (std::numeric_limits<int>    ::min)();
constexpr int64_t int64_max = (std::numeric_limits<int64_t>::max)();
constexpr int64_t int64_min = (std::numeric_limits<int64_t>::min)();
constexpr size_t  size_max  = (std::numeric_limits<size_t> ::max)();
constexpr size_t  size_min  = (std::numeric_limits<size_t> ::min)();

// --------------------
//      手番
// --------------------

// 手番
enum Color : int8_t { BLACK = 0/*先手*/, WHITE = 1/*後手*/, COLOR_NB /* = 2 */ , COLOR_ZERO = 0,};

// 相手番を返す
constexpr Color operator ~(Color c) { return (Color)(c ^ 1);  }

// 正常な値であるかを検査する。assertで使う用。
constexpr bool is_ok(Color c) { return COLOR_ZERO <= c && c < COLOR_NB; }

// 出力用(USI形式ではない)　デバッグ用。
std::ostream& operator<<(std::ostream& os, Color c);

// --------------------
//        筋
// --------------------

//  例) FILE_3なら3筋。
enum File : int8_t { FILE_1, FILE_2, FILE_3, FILE_4, FILE_5, FILE_6, FILE_7, FILE_8, FILE_9 , FILE_NB , FILE_ZERO=0 };

// 正常な値であるかを検査する。assertで使う用。
constexpr bool is_ok(File f) { return FILE_ZERO <= f && f < FILE_NB; }

// USIの指し手文字列などで筋を表す文字列をここで定義されたFileに変換する。
constexpr File toFile(char c) { return (File)(c - '1'); }

// Fileを綺麗に出力する(USI形式ではない)
// "PRETTY_JP"をdefineしていれば、日本語文字での表示になる。例 → ８
// "PRETTY_JP"をdefineしていなければ、数字のみの表示になる。例 → 8
std::string pretty(File f);

// USI形式でFileを出力する
static std::ostream& operator<<(std::ostream& os, File f) { os << (char)('1' + f); return os; }

// --------------------
//        段
// --------------------

// 例) RANK_4なら4段目。
enum Rank : int8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9 , RANK_NB , RANK_ZERO = 0};

// 正常な値であるかを検査する。assertで使う用。
constexpr bool is_ok(Rank r) { return RANK_ZERO <= r && r < RANK_NB; }

// 移動元、もしくは移動先の升のrankを与えたときに、そこが成れるかどうかを判定する。
constexpr bool canPromote(const Color c, const Rank fromOrToRank) {
	// ASSERT_LV1(is_ok(c) && is_ok(fromOrToRank));
	// 先手9bit(9段) + 後手9bit(9段) = 18bitのbit列に対して、判定すればいい。
	// ただし ×9みたいな掛け算をするのは嫌なのでbit shiftで済むように先手16bit、後手16bitの32bitのbit列に対して判定する。
	// このcastにおいて、VC++2015ではwarning C4800が出る。
	return static_cast<bool>(0x1c00007u & (1u << ((c << 4) + fromOrToRank)));
}

// 後手の段なら先手から見た段を返す。
// 例) relative_rank(WHITE,RANK_1) == RANK_9
constexpr Rank relative_rank(Color c, Rank r) { return c == BLACK ? r : (Rank)(8 - r); }

// USIの指し手文字列などで段を表す文字列をここで定義されたRankに変換する。
constexpr Rank toRank(char c) { return (Rank)(c - 'a'); }

// Rankを綺麗に出力する(USI形式ではない)
// "PRETTY_JP"をdefineしていれば、日本語文字での表示になる。例 → 八
// "PRETTY_JP"をdefineしていなければ、数字のみの表示になる。例 → 8
std::string pretty(Rank r);

// USI形式でRankを出力する
static std::ostream& operator<<(std::ostream& os, Rank r) { os << (char)('a' + r); return os; }

// --------------------
//        升目
// --------------------

// 盤上の升目に対応する定数。
// 盤上右上(１一が0)、左下(９九)が80
// 方角を表現するときにマイナスの値を使うので符号型である必要がある。
enum Square : int8_t
{
	// 以下、盤面の右上から左下までの定数。
	// これを定義していなくとも問題ないのだが、デバッガでSquare型を見たときに
	// どの升であるかが表示されることに価値がある。
	SQ_11, SQ_12, SQ_13, SQ_14, SQ_15, SQ_16, SQ_17, SQ_18, SQ_19,
	SQ_21, SQ_22, SQ_23, SQ_24, SQ_25, SQ_26, SQ_27, SQ_28, SQ_29,
	SQ_31, SQ_32, SQ_33, SQ_34, SQ_35, SQ_36, SQ_37, SQ_38, SQ_39,
	SQ_41, SQ_42, SQ_43, SQ_44, SQ_45, SQ_46, SQ_47, SQ_48, SQ_49,
	SQ_51, SQ_52, SQ_53, SQ_54, SQ_55, SQ_56, SQ_57, SQ_58, SQ_59,
	SQ_61, SQ_62, SQ_63, SQ_64, SQ_65, SQ_66, SQ_67, SQ_68, SQ_69,
	SQ_71, SQ_72, SQ_73, SQ_74, SQ_75, SQ_76, SQ_77, SQ_78, SQ_79,
	SQ_81, SQ_82, SQ_83, SQ_84, SQ_85, SQ_86, SQ_87, SQ_88, SQ_89,
	SQ_91, SQ_92, SQ_93, SQ_94, SQ_95, SQ_96, SQ_97, SQ_98, SQ_99,

	// ゼロと末尾
	SQ_ZERO = 0, SQ_NB = 81,
	SQUARE_NB = SQ_NB,       // Stockfishとの互換性維持のために定義
	SQ_NONE   = SQ_NB,       // Stockfishとの互換性維持のために定義
	SQ_NB_PLUS1 = SQ_NB + 1, // 玉がいない場合、SQ_NBに移動したものとして扱うため、配列をSQ_NB+1で確保しないといけないときがあるのでこの定数を用いる。

	// 方角に関する定数。StockfishだとNORTH=北=盤面の下を意味するようだが、
	// わかりにくいのでやねうら王ではストレートな命名に変更する。
	SQ_D = +1, // 下(Down)
	SQ_R = -9, // 右(Right)
	SQ_U = -1, // 上(Up)
	SQ_L = +9, // 左(Left)

	// 斜めの方角などを意味する定数。
	SQ_RU = int(SQ_U) + int(SQ_R), // 右上(Right Up)
	SQ_RD = int(SQ_D) + int(SQ_R), // 右下(Right Down)
	SQ_LU = int(SQ_U) + int(SQ_L), // 左上(Left Up)
	SQ_LD = int(SQ_D) + int(SQ_L), // 左下(Left Down)
	SQ_RUU = int(SQ_RU) + int(SQ_U), // 右上上
	SQ_LUU = int(SQ_LU) + int(SQ_U), // 左上上
	SQ_RDD = int(SQ_RD) + int(SQ_D), // 右下下
	SQ_LDD = int(SQ_LD) + int(SQ_D), // 左下下
};

// sqが盤面の内側を指しているかを判定する。assert()などで使う用。
// 駒は駒落ちのときにSQ_NBに移動するので、値としてSQ_NBは許容する。
constexpr bool is_ok(Square sq) { return SQ_ZERO <= sq && sq <= SQ_NB; }

// sqが盤面の内側を指しているかを判定する。assert()などで使う用。玉は盤上にないときにSQ_NBを取るのでこの関数が必要。
constexpr bool is_ok_plus1(Square sq) { return SQ_ZERO <= sq && sq < SQ_NB_PLUS1; }

// 与えられたSquareに対応する筋を返すテーブル。file_of()で用いる。
constexpr File SquareToFile[SQ_NB_PLUS1] =
{
	FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1, FILE_1,
	FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2, FILE_2,
	FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3, FILE_3,
	FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4, FILE_4,
	FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5, FILE_5,
	FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6, FILE_6,
	FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7, FILE_7,
	FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8, FILE_8,
	FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9, FILE_9,
	FILE_NB, // 玉が盤上にないときにこの位置に移動させることがあるので
};

// 与えられたSquareに対応する筋を返す。
// →　行数は長くなるが速度面においてテーブルを用いる。
constexpr File file_of(Square sq) { /* return (File)(sq / 9); */ /*ASSERT_LV2(is_ok(sq));*/ return SquareToFile[sq]; }

// 与えられたSquareに対応する段を返すテーブル。rank_of()で用いる。
constexpr Rank SquareToRank[SQ_NB_PLUS1] =
{
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9,
	RANK_NB, // 玉が盤上にないときにこの位置に移動させることがあるので
};

// 与えられたSquareに対応する段を返す。
// →　行数は長くなるが速度面においてテーブルを用いる。
constexpr Rank rank_of(Square sq) { /* return (Rank)(sq % 9); */ /*ASSERT_LV2(is_ok(sq));*/ return SquareToRank[sq]; }

// 筋(File)と段(Rank)から、それに対応する升(Square)を返す。
constexpr Square operator | (File f, Rank r) { Square sq = (Square)(f * 9 + r); /* ASSERT_LV2(is_ok(sq));*/ return sq; }

// ２つの升のfileの差、rankの差のうち大きいほうの距離を返す。sq1,sq2のどちらかが盤外ならint_maxが返る。
constexpr int dist(Square sq1, Square sq2) { return (!is_ok(sq1) || !is_ok(sq2)) ? int_max : (std::max)(abs(file_of(sq1) - file_of(sq2)), abs(rank_of(sq1) - rank_of(sq2))); }

// 移動元、もしくは移動先の升sqを与えたときに、そこが成れるかどうかを判定する。
constexpr bool canPromote(const Color c, const Square fromOrTo) {
	ASSERT_LV2(is_ok(fromOrTo));
	return canPromote(c, rank_of(fromOrTo));
}

// 移動元と移動先の升を与えて、成れるかどうかを判定する。
// (移動元か移動先かのどちらかが敵陣であれば成れる)
constexpr bool canPromote(const Color c, const Square from, const Square to)
{
  return canPromote(c, from) || canPromote(c, to);
}

// 盤面を180°回したときの升目を返す
constexpr Square Inv(Square sq) { return (Square)((SQ_NB - 1) - sq); }

// 盤面をミラーしたときの升目を返す
// (5筋を軸としたミラーであって、玉の筋を軸としたミラーとかではない。)
constexpr Square Mir(Square sq) { return File(8-(int)file_of(sq)) | rank_of(sq); }

// 盤面を180度回転させた時の升を返す。
constexpr Square Flip(Square sq) { return (Square)(SQ_NB - sq - 1); }

// Squareを綺麗に出力する(USI形式ではない)
// "PRETTY_JP"をdefineしていれば、日本語文字での表示になる。例 → ８八
// "PRETTY_JP"をdefineしていなければ、数字のみの表示になる。例 → 88
static std::string pretty(Square sq) { return pretty(file_of(sq)) + pretty(rank_of(sq)); }


// USI形式でSquareを出力する
static std::ostream& operator<<(std::ostream& os, Square sq) { os << file_of(sq) << rank_of(sq); return os; }

// --------------------
//   壁つきの升表現
// --------------------

// This trick is invented by yaneurao in 2016.

// 長い利きを更新するときにある升からある方向に駒にぶつかるまでずっと利きを更新していきたいことがあるが、
// sqの升が盤外であるかどうかを判定する簡単な方法がない。そこで、Squareの表現を拡張して盤外であることを検出
// できるようにする。

// bit 0..7   : Squareと同じ意味
// bit 8      : Squareからのborrow用に1にしておく
// bit 9..13  : いまの升から盤外まで何升右に升があるか(ここがマイナスになるとborrowでbit13が1になる)
// bit 14..18 : いまの升から盤外まで何升上に(略
// bit 19..23 : いまの升から盤外まで何升下に(略
// bit 24..28 : いまの升から盤外まで何升左に(略
enum SquareWithWall : int32_t {
	// 相対移動するときの差分値
	SQWW_R = SQ_R - (1 << 9) + (1 << 24), SQWW_U = SQ_U - (1 << 14) + (1 << 19), SQWW_D = -int(SQWW_U), SQWW_L = -int(SQWW_R),
	SQWW_RU = int(SQWW_R) + int(SQWW_U), SQWW_RD = int(SQWW_R) + int(SQWW_D), SQWW_LU = int(SQWW_L) + int(SQWW_U), SQWW_LD = int(SQWW_L) + int(SQWW_D),

	// SQ_11の地点に対応する値(他の升はこれ相対で事前に求めテーブルに格納)
	SQWW_11 = SQ_11 | (1 << 8) /* bit8 is 1 */ | (0 << 9) /*右に0升*/ | (0 << 14) /*上に0升*/ | (8 << 19) /*下に8升*/ | (8 << 24) /*左に8升*/,

	// SQWW_RIGHTなどを足して行ったときに盤外に行ったときのborrow bitの集合
	SQWW_BORROW_MASK = (1 << 13) | (1 << 18) | (1 << 23) | (1 << 28),
};

// 型変換。下位8bit == Square
constexpr Square sqww_to_sq(SquareWithWall sqww) { return Square(sqww & 0xff); }

// to_sqww()で使うテーブル。直接アクセスしないようにnamespaceに入れてある。
namespace BB_Table { extern SquareWithWall sqww_table[SQ_NB_PLUS1]; }

// 型変換。Square型から。
static SquareWithWall to_sqww(Square sq) { return BB_Table::sqww_table[sq]; }

// 盤内か。壁(盤外)だとfalseになる。
constexpr bool is_ok(SquareWithWall sqww) { return (sqww & SQWW_BORROW_MASK) == 0; }

// 単にSQの升を出力する。
static std::ostream& operator<<(std::ostream& os, SquareWithWall sqww) { os << sqww_to_sq(sqww); return os; }

// --------------------
//        方角
// --------------------

// Long Effect Libraryの一部。これは8近傍、24近傍の利きを直列化したり方角を求めたりするライブラリ。
// ここではその一部だけを持って来た。(これらは頻繁に用いるので)
// それ以上を使いたい場合は、LONG_EFFECT_LIBRARYというシンボルをdefineして、extra/long_effect.hをincludeすること。
namespace Effect8
{
	// 方角を表す。遠方駒の利きや、玉から見た方角を表すのに用いる。
	// bit0..右上、bit1..右、bit2..右下、bit3..上、bit4..下、bit5..左上、bit6..左、bit7..左下
	// 同時に複数のbitが1であることがありうる。
	enum Directions : uint8_t {
		DIRECTIONS_ZERO  = 0, DIRECTIONS_RU = 1, DIRECTIONS_R = 2, DIRECTIONS_RD = 4,
		DIRECTIONS_U     = 8, DIRECTIONS_D = 16, DIRECTIONS_LU = 32, DIRECTIONS_L = 64, DIRECTIONS_LD = 128,
		DIRECTIONS_CROSS = DIRECTIONS_U  | DIRECTIONS_D  | DIRECTIONS_R  | DIRECTIONS_L,
		DIRECTIONS_DIAG  = DIRECTIONS_RU | DIRECTIONS_RD | DIRECTIONS_LU | DIRECTIONS_LD,
	};

	// sq1にとってsq2がどのdirectionにあるか。
	// "Direction"ではなく"Directions"を返したほうが、縦横十字方向や、斜め方向の位置関係にある場合、
	// DIRECTIONS_CROSSやDIRECTIONS_DIAGのような定数が使えて便利。
	extern Directions direc_table[SQ_NB_PLUS1][SQ_NB_PLUS1];
	static Directions directions_of(Square sq1, Square sq2) { return direc_table[sq1][sq2]; }

	// Directionsをpopしたもの。複数の方角を同時に表すことはない。
	// おまけで桂馬の移動も追加しておく。
	enum Direct {
		DIRECT_RU, DIRECT_R, DIRECT_RD, DIRECT_U, DIRECT_D, DIRECT_LU, DIRECT_L, DIRECT_LD,
		DIRECT_NB, DIRECT_ZERO = 0, DIRECT_RUU = 8, DIRECT_LUU, DIRECT_RDD, DIRECT_LDD, DIRECT_NB_PLUS4
	};

	// Directionsに相当するものを引数に渡して1つ方角を取り出す。
	static Direct pop_directions(Directions& d) { return (Direct)pop_lsb(d); }

	// sq1にとってsq2がどの方角であるかを返す。
	// sq1がsq2に対して八方向のいずれかであることがわかっている時に用いる。
	static Direct direct_of(Square sq1, Square sq2)
	{
		auto d = directions_of(sq1, sq2);
		ASSERT_LV3(d != DIRECTIONS_ZERO);
		return pop_directions(d);
	}

	// ある方角の反対の方角(180度回転させた方角)を得る。
	constexpr Direct operator~(Direct d) {
		// Directの定数値を変更したら、この関数はうまく動作しない。
		static_assert(Effect8::DIRECT_R == 1, "");
		static_assert(Effect8::DIRECT_L == 6, "");
		// DIRECT_RUUなどは引数に渡してはならない。
		// ASSERT_LV3(d < DIRECT_NB);
		return Direct(7 - d);
	}

	// DirectからDirectionsへの逆変換
	constexpr Directions to_directions(Direct d) { return Directions(1 << d); }

	constexpr bool is_ok(Direct d) { return DIRECT_ZERO <= d && d < DIRECT_NB_PLUS4; }

	// DirectをSquareWithWall型の差分値で表現したもの。
	constexpr SquareWithWall DirectToDeltaWW_[DIRECT_NB] = { SQWW_RU,SQWW_R,SQWW_RD,SQWW_U,SQWW_D,SQWW_LU,SQWW_L,SQWW_LD, };
	constexpr SquareWithWall DirectToDeltaWW(Direct d) { /* ASSERT_LV3(is_ok(d)); */ return DirectToDeltaWW_[d]; }
}

// 与えられた3升が縦横斜めの1直線上にあるか。駒を移動させたときに開き王手になるかどうかを判定するのに使う。
// 例) 王がsq1, pinされている駒がsq2にあるときに、pinされている駒をsq3に移動させたときにaligned(sq1,sq2,sq3)であれば、
//  pinされている方向に沿った移動なので開き王手にはならないと判定できる。
// ただし玉はsq3として、sq1,sq2は同じ側にいるものとする。(玉を挟んでの一直線は一直線とはみなさない)
static bool aligned(Square sq1, Square sq2, Square sq3/* is ksq */)
{
	auto d1 = Effect8::directions_of(sq1, sq3);
	return d1 && d1 == Effect8::directions_of(sq2, sq3);
}

// --------------------
//     探索深さ
// --------------------

// 通常探索時の最大探索深さ
constexpr int MAX_PLY = MAX_PLY_NUM;

// 探索深さを表現する型
using Depth = int;

// The following DEPTH_ constants are used for TT entries and QS movegen stages. In regular search,
// TT depth is literal: the search depth (effort) used to make the corresponding TT value.
// In qsearch, however, TT entries only store the current QS movegen stage (which should thus compare
// lower than any regular search depth).
// 静止探索で王手がかかっているときにこれより少ない残り探索深さでの探索した結果が置換表にあってもそれは信用しない
constexpr Depth DEPTH_QS = 0;

// For TT entries where no searching at all was done (whether regular or qsearch) we use
// _UNSEARCHED, which should thus compare lower than any QS or regular depth. _ENTRY_OFFSET is used
// only for the TT entry occupancy check (see tt.cpp), and should thus be lower than _UNSEARCHED.

// DEPTH_NONEは探索せずに値を求めたという意味に使う。
constexpr Depth DEPTH_UNSEARCHED   = -2;

// TTの下駄履き用(TTEntryが使われているかどうかのチェックにのみ用いる)
constexpr Depth DEPTH_ENTRY_OFFSET = -3;

// --------------------
//     評価値の性質
// --------------------

// searchで探索窓を設定するので、この窓の範囲外の値が返ってきた場合、
// high fail時はこの値は上界(真の値はこれより小さい)、low fail時はこの値は下界(真の値はこれより大きい)
// である。
enum Bound : int8_t {
	BOUND_NONE,  // 探索していない(DEPTH_NONE)ときに、最善手か、静的評価スコアだけを置換表に格納したいときに用いる。
	BOUND_UPPER, // 上界(真の評価値はこれより小さい) = 詰みのスコアや、nonPVで評価値があまり信用ならない状態であることを表現する。
	BOUND_LOWER, // 下界(真の評価値はこれより大きい)
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER // 真の評価値と一致している。PV nodeでかつ詰みのスコアでないことを表現する。
};

// --------------------
//        評価値
// --------------------

// 置換表に格納するときにあまりbit数が多いともったいないので値自体は16bitで収まる範囲で。
// 
// ■ 注意
//
//	1. 評価値evalは、
//       Stockfishでは、  VALUE_TB_LOSS_IN_MAX_PLY < eval < VALUE_TB_WIN_IN_MAX_PLY の範囲。
//		 やねうら王でも同様。
//       やねうら王では、 VALUE_MIN_EVAL <= eval <= VALUE_MAX_EVALの範囲とも言える。
//			(もしくは、abs(eval) <= VALUE_MAX_EVALの範囲。)
//  2. 詰みのスコアの下限値は、
//       Stockfishでは、  VALUE_TB_WIN_IN_MAX_PLY
//		 やねうら王でも同様。
//  3. 詰まされスコアの上限値は、
//		 Stockfishでは、  VALUE_TB_LOSS_IN_MAX_PLY
//		 やねうら王でも同様。
//
// Value is used as an alias for int, this is done to differentiate between a search
// value and any other integer value. The values used in search are always supposed
// to be in the range (-VALUE_NONE, VALUE_NONE] and should not exceed this range.

// Valueはint型のエイリアスとして使用されています。これは、検索に使用される値と
// その他の整数値を区別するために行われています。検索で使用される値は
// 常に(-VALUE_NONE, VALUE_NONE]の範囲内である必要があり、この範囲を超えてはなりません。

using Value = int;

constexpr Value VALUE_ZERO     = 0;
// 引き分け時のスコア(千日手のスコアリングなどで用いる)
constexpr Value VALUE_DRAW     = 0;

// 無効な値
constexpr Value VALUE_NONE     = 32002;

// Valueの取りうる最大値(最小値はこの符号を反転させた値)
constexpr Value VALUE_INFINITE = 32001;

// 0手詰めのスコア(rootで詰んでいるときのscore)
// 例えば、3手詰めならこの値より3少ない。
constexpr Value VALUE_MATE             = 32000;

constexpr Value VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY;  // MAX_PLYでの詰みのときのスコア。
constexpr Value VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY; // MAX_PLYで詰まされるときのスコア。

// チェスの終盤DB(tablebase)によって得られた詰みを表現するスコアらしいが、
// Stockfishとの互換性のためにこのシンボルはStockfishと同様に定義しておく。
constexpr Value VALUE_TB                 =  VALUE_MATE_IN_MAX_PLY - 1;
constexpr Value VALUE_TB_WIN_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY;
constexpr Value VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

// 千日手による優等局面への突入したときのスコア
// ※ これを詰みのスコアの仲間としてしまうと、詰みのスコアをrootからの手数で
//    計算しなおすときにおかしくなる。これは評価値の仲間として扱うことにする。
//   よって、これは、VALUE_MAX_EVALと同じ値にしておく。
constexpr Value VALUE_SUPERIOR =  VALUE_TB_WIN_IN_MAX_PLY - 1;

// 評価関数の返す値の最大値、最小値
constexpr Value VALUE_MAX_EVAL =  VALUE_SUPERIOR;
constexpr Value VALUE_MIN_EVAL = -VALUE_MAX_EVAL;

// 評価関数がまだ呼び出されていないということを示すのに使う特殊な定数
// StateInfo::sum.p[0][0]にこの値を格納して、マーカーとするのだが、このsumのp[0][0]は、ΣBKPPの計算結果であり、
// 16bitの範囲で収まるとは限らないため、もっと大きな数にしておく必要がある。
constexpr Value VALUE_NOT_EVALUATED = INT32_MAX;

// ply手で詰ませるときのスコア
constexpr Value mate_in(int ply) { return (Value)(VALUE_MATE - ply); }

// ply手で詰まされるときのスコア
constexpr Value mated_in(int ply) { return (Value)(-VALUE_MATE + ply); }


// ValueがVALUE_NONEでなければtrue。
constexpr bool is_valid(Value value) { return value != VALUE_NONE; }

// Valueが、VALUE_TB_WIN_IN_MAX_PLY以上(詰み確定のスコア)であればtrue。
constexpr bool is_win(Value value) {
	assert(is_valid(value));
	return value >= VALUE_TB_WIN_IN_MAX_PLY;
}

// Valueが詰まされ確定のスコアより低いならtrue。
constexpr bool is_loss(Value value) {
	assert(is_valid(value));
	return value <= VALUE_TB_LOSS_IN_MAX_PLY;
}

// Valueが詰み/詰まされ確定のスコアならtrue。
constexpr bool is_decisive(Value value) { return is_win(value) || is_loss(value); }


// --------------------
//        駒
// --------------------

// USIプロトコルでやりとりするときの駒の表現
extern const char* USI_PIECE;

// 駒の種類(先後の区別なし)
enum PieceType : int8_t
{
	// 金の順番を飛の後ろにしておく。KINGを8にしておく。
	// こうすることで、成りを求めるときに pc |= 8;で求まり、かつ、先手の全種類の駒を列挙するときに空きが発生しない。(DRAGONが終端になる)
	NO_PIECE_TYPE, PAWN/*歩*/, LANCE/*香*/, KNIGHT/*桂*/, SILVER/*銀*/, BISHOP/*角*/, ROOK/*飛*/, GOLD/*金*/,
	KING = 8/*玉*/, PRO_PAWN /*と*/, PRO_LANCE /*成香*/, PRO_KNIGHT /*成桂*/, PRO_SILVER /*成銀*/, HORSE/*馬*/, DRAGON/*龍*/, QUEEN/*未使用*/,

	PIECE_HAND_ZERO = PAWN, // 手駒の開始位置
	PIECE_HAND_NB = KING,   // 手駒になる駒種の最大+1

	PIECE_TYPE_PROMOTE = 8, // 成り駒と非成り駒との差(この定数を足すと成り駒になる)
	PIECE_TYPE_NB = 16, // 駒種の数。(成りを含める)

	// --- 特殊用途

	// 指し手生成(GeneratePieceMove = GPM)でtemplateの引数として使うマーカー的な値。変更する可能性があるのでユーザーは使わないでください。
	// 連続的な値にしておくことで、テーブルジャンプしやすくする。
	GPM_BR = 16,     // Bishop Rook
	GPM_GBR = 17,     // Gold Bishop Rook
	GPM_GHD = 18,     // Gold Horse Dragon
	GPM_GHDK = 19,     // Gold Horse Dragon King

	// --- Position::pieces()で用いる特殊な定数。空いてるところを順番に用いる。
	// Position::pieces()では、PAWN , LANCE , … , DRAGONはそのまま用いるが、それ以外に↓の定数が使える。
	ALL_PIECES = 0,			// 駒がある升を示すBitboardが返る。
	GOLDS = QUEEN,			// 金と同じ移動特性を持つ駒のBitboardが返る。
	HDK,				    // H=Horse,D=Dragon,K=Kingの合体したBitboardが返る。
	BISHOP_HORSE,			// BISHOP,HORSEを合成したBitboardが返る。
	ROOK_DRAGON,			// ROOK,DRAGONを合成したBitboardが返る。
	SILVER_HDK,				// SILVER,HDKを合成したBitboardが返る。
	GOLDS_HDK,				// GOLDS,HDKを合成したBitboardが返る。
	PIECE_BB_NB,			// 終端
};

// 駒(先後の区別あり)
enum Piece : int8_t
{
	NO_PIECE = 0,

	// 以下、先後の区別のある駒(Bがついているのは先手、Wがついているのは後手)
	B_PAWN = 1 , B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING, B_PRO_PAWN, B_PRO_LANCE, B_PRO_KNIGHT, B_PRO_SILVER, B_HORSE, B_DRAGON, B_GOLDS/*金相当の駒*/,
	W_PAWN = 17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING, W_PRO_PAWN, W_PRO_LANCE, W_PRO_KNIGHT, W_PRO_SILVER, W_HORSE, W_DRAGON, W_GOLDS/*金相当の駒*/,
	PIECE_NB, // 終端
	PIECE_ZERO = 0,

	// --- 特殊な定数

	PIECE_PROMOTE = 8, // 成り駒と非成り駒との差(この定数を足すと成り駒になる)
	PIECE_WHITE = 16,  // これを先手の駒に加算すると後手の駒になる。
	PIECE_RAW_NB = 8,  // 非成駒の終端
};

// USIプロトコルで駒を表す文字列を返す。
// 駒打ちの駒なら先頭に"D"。
static std::string usi_piece(Piece pc) { return (pc & 32) ? "D":""
		  + std::string(USI_PIECE).substr((pc & 31) * 2, 2); }

// 駒に対して、それが先後、どちらの手番の駒であるかを返す。
constexpr Color color_of(Piece pc)
{
//	return (pc & PIECE_WHITE) ? WHITE : BLACK;
	static_assert(PIECE_WHITE == 16 && WHITE == 1 && BLACK == 0, "");
	return (Color)((pc & PIECE_WHITE) >> 4);
}

// 後手の歩→先手の歩のように、後手という属性を取り払った(先後の区別をなくした)駒種を返す
constexpr PieceType type_of(Piece pc) { return (PieceType)(pc & 15); }

// 駒に対してこれ以上成れない駒かどうかを判定する。
// (成り駒はもちろん、玉、金に対してもtrueが返るので注意すること)
constexpr bool is_non_promotable_piece(Piece pc)
{
	static_assert(GOLD == 7, "GOLD must be 7.");
	return type_of(pc) >= GOLD;
}

// 成り駒か判定する。
// 玉は成り駒ではないのでfalseが返る。「と」「成香」…に対してtrueが返る。
constexpr bool is_promoted(Piece pc)
{
	return type_of(pc) >= PRO_PAWN;
}

// 成ってない駒を返す。後手という属性も消去する。
// 例) 成銀→銀 , 後手の馬→先手の角
// ただし、pc == KINGでの呼び出しはNO_PIECEが返るものとする。
constexpr PieceType raw_type_of(Piece pc) { return (PieceType)(pc & 7); }

// 成ってない駒を返す。
// 例) 成銀→銀 , 後手の馬→後手の角
// ただし、pc == KINGでの呼び出しはNO_PIECEが返るものとする。
constexpr Piece raw_of(Piece pc) { return (Piece)(pc & ~8); }

// pcとして先手の駒を渡し、cが後手なら後手の駒を返す。cが先手なら先手の駒のまま。pcとしてNO_PIECEは渡してはならない。
constexpr Piece make_piece(Color c, PieceType pt) { /*ASSERT_LV3(color_of(pt) == BLACK && pt!=NO_PIECE); */ return (Piece)((c << 4) + pt); }

// 成り駒を返す。与えられたpcが成り駒の場合はそのまま返す。
constexpr Piece make_promoted_piece(Piece pc) { return (Piece)(pc | PIECE_PROMOTE); }

// pcが遠方駒であるかを判定する。LANCE,BISHOP(5),ROOK(6),HORSE(13),DRAGON(14)
constexpr bool has_long_effect(Piece pc) { return (type_of(pc) == LANCE) || (((pc+1) & 6)==6); }

// Pieceの整合性の検査。assert用。
// Pieceはuintなので "NO_PIECE <= pc"は意味をなさない比較なのでコメントアウトしてある。(コンパイラの警告がでる)
constexpr bool is_ok(Piece pc) { return /* NO_PIECE <= pc && */ pc < PIECE_NB; }

// Pieceを綺麗に出力する(USI形式ではない) 先手の駒は大文字、後手の駒は小文字、成り駒は先頭に+がつく。盤面表示に使う。
// "PRETTY_JP"をdefineしていれば、日本語文字での表示になる。
std::string pretty(Piece pc);
static std::string pretty(PieceType pt) { return pretty((Piece)pt); }

// ↑のpretty()だと先手の駒を表示したときに先頭にスペースが入るので、それが嫌な場合はこちらを用いる。
static std::string pretty2(Piece pc) { ASSERT_LV1(color_of(pc) == BLACK); auto s = pretty(pc); return s.substr(1, s.length() - 1); }
static std::string pretty2(PieceType pt) { return pretty2((Piece)pt); }

// USIで、盤上の駒を表現する文字列
// ※　歩Pawn 香Lance 桂kNight 銀Silver 角Bishop 飛Rook 金Gold 王King
extern std::string PieceToCharBW;

// Piece,PieceTypeをUSI形式で表示する。
std::ostream& operator<<(std::ostream& os, Piece pc);
static std::ostream& operator<<(std::ostream& os, PieceType pc) { return os << (Piece)pc; }

// --------------------
//        駒箱
// --------------------

// Positionクラスで用いる、駒リスト(どの駒がどこにあるのか)を管理するときの番号。
enum PieceNumber : u8
{
  PIECE_NUMBER_PAWN = 0, PIECE_NUMBER_LANCE = 18, PIECE_NUMBER_KNIGHT = 22, PIECE_NUMBER_SILVER = 26,
  PIECE_NUMBER_GOLD = 30, PIECE_NUMBER_BISHOP = 34, PIECE_NUMBER_ROOK = 36, PIECE_NUMBER_KING = 38,
  PIECE_NUMBER_BKING = 38, PIECE_NUMBER_WKING = 39, // 先手、後手の玉の番号が必要な場合はこっちを用いる
  PIECE_NUMBER_ZERO = 0, PIECE_NUMBER_NB = 40,
};

// PieceNumberの整合性の検査。assert用。
constexpr bool is_ok(PieceNumber pn) { return pn < PIECE_NUMBER_NB; }

// --------------------
//       指し手
// --------------------

// Based on a congruential pseudo-random number generator
// 合同式による疑似乱数生成器に基づいています。
// ⇨ Move型のhashを生成するときに用いる。

constexpr /*Key*/size_t make_key(uint64_t seed) {
	return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

class Move;
class Move16;

// USI形式の文字列にする。
// USI::move(Move m)と同等。互換性のために残されている。
std::string to_usi_string(Move m);
std::string to_usi_string(Move16 m);

// Move16 : 16bit形式の指し手
//   bit0..6  : 移動先のSquare
//   bit7..13 : 移動元のSquare
//				駒打ちのときはPieceType。例: 歩打ちの時は、ここのbitは、0001b、香打ちの時は0010b。
//   bit14    : 駒打ちかのフラグ
//   bit15    : 成りかのフラグ
//   
// Move   : 32bit形式の指し手
//   上位16bitには、この指し手によってto(移動後の升)に来る駒(先後の区別あり)が格納されている。
//   つまりは Piece(5bit)が上位16bitに来る。
//   move = move16 + (piece << 16)
// なので、Moveが使うのは、16bit(Move16) + 5bit(Piece) = 下位21bit
//

enum MoveEnum : uint32_t {
	MOVE_NONE    = 0,             // 無効な移動

	MOVE_NULL    = (1 << 7) + 1,  // NULL MOVEを意味する指し手。Square(1)からSquare(1)への移動は存在しないのでここを特殊な記号として使う。
	MOVE_RESIGN  = (2 << 7) + 2,  // << で出力したときに"resign"と表示する投了を意味する指し手。
	MOVE_WIN     = (3 << 7) + 3,  // 入玉時の宣言勝ちのために使う特殊な指し手

	MOVE_DROP    = 1 << 14,       // 駒打ちフラグ
	MOVE_PROMOTE = 1 << 15,       // 駒成りフラグ
};

// 指し手を表現するclass(実体は32bit整数)
class Move
{
public:
	Move() = default;
	constexpr explicit Move(std::uint32_t d): data(d) {}

	// -- 定数

	// MOVE_NONEに相当する定数。
	static constexpr Move none() { return Move(MOVE_NONE); }

	// MOVE_NULLに相当する定数。
	static constexpr Move null() { return Move(MOVE_NULL); }

	// MOVE_RESIGNに相当する定数。
	static constexpr Move resign() { return Move(MOVE_RESIGN); }

	// MOVE_WINに相当する定数。
	static constexpr Move win() { return Move(MOVE_WIN); }

	// -- property

	// 指し手の移動元の升を返す。
	// → ここ、ASSERT使っててconstexpr式として評価できないかも？
	Square from_sq() const {
		ASSERT_LV3(is_ok());
		return Square((data >> 7) & 0x7f);
	}

	// 指し手の移動先の升を返す。
	Square to_sq() const { return Square(data & 0x7f); }

	// 指し手が駒打ちか？
	bool is_drop() const { return (data & MOVE_DROP) != 0; }

	// 指し手が成りか？
	bool is_promote() const { return (data & MOVE_PROMOTE) != 0; }

	// 駒打ち(is_drop()==true)のときの打った駒
	// 先後の区別なし。PAWN～ROOKまでの値が返る。
	// ※ 打つ駒のPieceTypeはMoveの bit7..13に格納されている。
	// ※ assert(is_drop(m))はあってもいいかも。
	PieceType move_dropped_piece() const { return PieceType((data >> 7) & 0x7f); }

	// この指し手のあとにtoに来る駒。(移動させる駒だが、成りのときは、成ったあとの駒。)
	Piece moved_after_piece() const { return Piece(data >> 16); }

	// fromとtoをシリアライズする。駒打ちのときのfromは普通の移動の指し手とは異なる。
	// この関数は、0 ～ ((SQ_NB+7) * SQ_NB - 1)までの値が返る。
	// ※ is_drop() == trueの時、from_sq(m)は、打つ駒のPieceTypeが返る。NO_PIECE = 0で、ここが空番であることに注意。
	//    ゆえに、is_drop()==trueの時は、from_sq(m)にSQ_NB-1を足して、打つ駒がPAWN(= 1)の時にSQ_NBになるようにしてやる必要がある。
	// 注) 駒打ちに関して、先手の駒と後手の駒の区別はしない。
	// 　　これは、この関数は、MovePickerのButterflyHistoryで使うから必要なのだが、そこでは指し手の手番(Color)を別途持っているから。
	int from_to() const { return int(from_sq() + int(is_drop() ? (SQ_NB - 1) : 0)) * int(SQ_NB) + int(to_sq());}

	// 上記のfrom_toが返す最大値 + 1。
	static constexpr int FROM_TO_SIZE = int(SQ_NB + 7) * int(SQ_NB);

	// 指し手が普通の指し手(駒打ち/駒成り含む)であるかテストする。
	// 特殊な指し手(MOVE_NONE/MOVE_NULL/MOVE_WIN)である場合、falseが返る。
	// それ普通の指し手ならばtrueが返る。
	constexpr bool is_ok() const {
		// 通常の指し手ならば、
		// 1. 32bit moveの場合は、上位16bitに移動させる駒があるからm >> 7が下位7bitと一致することはない。
		// 2. 16bit moveの場合は、
		//   2a. 移動させる指し手ならば from == toは満たさない。
		//   2b. 駒打ちの場合は、m >> 7のbit7が1になっているので右辺とは一致しない。
		// ということで、以下の条件式で良い。
		return (data >> 7) != (data & 0x7f);
	}

	// -- 比較

	bool operator == (const Move rhs) const { return this->to_u32() == rhs.to_u32(); }
	bool operator != (const Move rhs) const { return !(*this == rhs); }

	// -- 変換子

	// Move16への変換子
	Move16 to_move16() const;
	constexpr uint16_t to_u16() const { return (uint16_t)data; }
	constexpr uint32_t to_u32() const { return (uint32_t)data; }
	constexpr explicit operator bool() const { return data != 0; }
	constexpr explicit operator uint32_t() const { return (uint32_t)data; }

	// -- 文字列化

	// USI形式の文字列にする。
	std::string to_usi_string() const { return YaneuraOu::to_usi_string(*this); }

	// -- unordered_mapなどで比較するときに用いる。operator<()は定義したくないので、こちらを用いる。
	struct MoveHash {
		std::size_t operator()(const Move& m) const { return make_key(m.data); }
	};

protected:
	std::uint32_t data;
};

// 16bit型の指し手。Move型の下位16bit。
// 32bit型のMoveか16bit型のMoveかが不明瞭で、それがバグの原因となりやすいので
// それらを明確に区別したい時に用いる。
class Move16
{
public:

	constexpr Move16() : data(0) {}
	explicit constexpr Move16(u16 m) : data(m) {}

	// -- 定数

	// MOVE_NONEに相当する定数。
	static constexpr Move16 none() { return Move16(MOVE_NONE); }

	// MOVE_NULLに相当する定数。
	static constexpr Move16 null() { return Move16(MOVE_NULL); }

	// MOVE_RESIGNに相当する定数。
	static constexpr Move16 resign() { return Move16(MOVE_RESIGN); }

	// MOVE_WINに相当する定数。
	static constexpr Move16 win() { return Move16(MOVE_WIN); }

	// -- property

	Square from_sq() const { ASSERT_LV3(is_ok()); return Square((data >> 7) & 0x7f); }
	Square to_sq() const { return Square(data & 0x7f); }
	bool is_drop() const { return (data & MOVE_DROP) != 0; }
	bool is_promote() const { return (data & MOVE_PROMOTE) != 0; }
	PieceType move_dropped_piece() const { return PieceType((data >> 7) & 0x7f); }

	int from_to() const { return int(from_sq() + int(is_drop() ? (SQ_NB - 1) : 0)) * int(SQ_NB) + int(to_sq()); }
	constexpr bool is_ok() const { return (data >> 7) != (data & 0x7f); }

	// -- 比較

	// Move16同士とMoveの定数とも比較はできる。
	bool operator == (const Move16 rhs) const { return data == rhs.data; }
	bool operator != (const Move16 rhs) const { return !(*this == rhs); }
	bool operator == (const Move rhs) const { return this->to_u16() == rhs.to_u16(); }
	bool operator != (const Move rhs) const { return !(*this == rhs); }

	// -- 変換子

	// uint16_tのまま取り出す。
	// Moveに変換が必要なときは、そのあとMove()にcastすることはできる。(上位16bitは0のまま)
	// 内部的に用いる。基本的にはこの関数を呼び出さないこと。
	// ⇨ このMove16のinstanceをMoveに戻したい時は、Position::to_move(Move16)を使うこと。
	constexpr uint16_t to_u16() const { return (u16)data; }

	//constexpr operator bool() const { return data; }
	// ⇨ これ定義していると余計なバグに悩まされることになる。

	// -- 文字列化

	// USI形式の文字列にする。
	std::string to_usi_string() const { return YaneuraOu::to_usi_string(*this); }

protected:
	uint16_t data;
};

// USI形式で指し手を表示する
static std::ostream& operator<<(std::ostream& os, Move m)   { os << to_usi_string(m); return os; }
static std::ostream& operator<<(std::ostream& os, Move16 m) { os << to_usi_string(m); return os; }


// us側のptをfromからtoに移動させる指し手を生成して返す。
// Move16を返すほうは、移動させる駒が何かの情報は持っていない。
static Move16 make_move16(Square from, Square to) { return Move16(u32(to) + (u32(from) << 7)); }
constexpr Move make_move(Square from, Square to, Piece pc ) { return Move(u32(to) + (u32(from) << 7) + (u32(pc) << 16)) ; }
constexpr Move make_move(Square from, Square to, Color us , PieceType pt ) { return Move(u32(to) + u32(from << 7) + (((us ? u32(PIECE_WHITE) : 0) + u32(pt)) << 16)) ; }

// us側の駒ptをfromからtoに移動して、成る指し手を生成して返す。
// Move16を返すほうは、移動させる駒が何かの情報は持っていない。
static Move16 make_move_promote16(Square from, Square to) { return Move16(u32(to) + (u32(from) << 7) + MOVE_PROMOTE); }
constexpr Move make_move_promote(Square from, Square to , Piece pc) { return Move(u32(to) + (u32(from) << 7) + MOVE_PROMOTE + ((pc | PIECE_PROMOTE) << 16)); }
constexpr Move make_move_promote(Square from, Square to , Color us , PieceType pt) { return Move(u32(to) + (u32(from) << 7) + MOVE_PROMOTE + (((us ? u32(PIECE_WHITE) : 0) + u32(pt | PIECE_PROMOTE)) << 16)); }

// us側の駒ptをtoに打つ指し手を生成して返す
// Move16を返すほうは、どちらの駒であるかの情報は持っていない。
static Move16 make_move_drop16(PieceType pt, Square to) { return Move16(u32(to) + (u32(pt) << 7) + MOVE_DROP); }
constexpr Move make_move_drop(PieceType pt, Square to , Color us ) { return Move(u32(to) + (u32(pt) << 7) + MOVE_DROP + (((us ? u32(PIECE_WHITE) : 0) + u32(pt)) << 16)); }

// 移動元の升と移動先の升を逆転させた指し手を生成する。探索部で用いる。
// ※　最新のStockfishでは使っていないのでもはや不要なのだが一応残しておく。
// 将棋はチェスと違って元の場所に戻れない駒が多いのでreverse_move()を用いるhistory updateは
// 大抵、悪影響しかない。
// また、reverse_move()を用いるならば、ifの条件式に " && !is_drop(move)"が要ると思う。
static Move16 reverse_move(Move m) { return make_move16(m.to_sq(), m.from_sq()); }

// 指し手を反転させる(盤面を180°回転させた指し手にする)
// 駒打ちもちゃんと考慮する。mがMOVE_NONEでも良い。(MOVE_NONEが返る)
// ※　この関数はやねうら王独自拡張。
//   定跡のprobeで180°反転させた盤面にもhitして欲しいのでそのヘルパー関数として追加した。
static Move16 flip_move(Move16 m) {

	return 
		m.is_drop   ()  ? make_move_drop16   (m.move_dropped_piece(), Flip(m.to_sq())):
        m.is_promote()  ? make_move_promote16(Flip(m.from_sq())     , Flip(m.to_sq())):
					      make_move16        (Flip(m.from_sq())     , Flip(m.to_sq()));
}

// 見た目に、わかりやすい形式で表示する
std::string pretty(Move m);

// 移動させた駒がわかっているときに指し手をわかりやすい表示形式で表示する。
std::string pretty(Move m, Piece movedPieceType);
static std::string pretty(Move m, PieceType movedPieceType) { return pretty(m, (Piece)movedPieceType); }

// Stockfishとの互換性を保つために導入。
// 普通の指し手か成りの指し手かを判定するのに用いる。
enum MoveType {
	NORMAL,
	PROMOTION = MOVE_PROMOTE,
	DROP      = MOVE_DROP,
	//ENPASSANT = 2 << 14,
	//CASTLING = 3 << 14
};

// 指し手(Move)のMoveTypeを返す。
constexpr MoveType type_of(Move m) { return MoveType(m.to_u16() & (MOVE_PROMOTE | MOVE_DROP)); }


// --------------------
//       手駒
// --------------------

// 手駒
// 歩の枚数を8bit、香、桂、銀、角、飛、金を4bitずつで持つ。こうすると16進数表示したときに綺麗に表示される。(なのはのアイデア)
enum Hand : uint32_t { HAND_ZERO = 0, };

// 手駒のbit位置
constexpr int PIECE_BITS[PIECE_HAND_NB] = { 0, 0 /*歩*/, 8 /*香*/, 12 /*桂*/, 16 /*銀*/, 20 /*角*/, 24 /*飛*/ , 28 /*金*/ };

// PieceType(歩,香,桂,銀,金,角,飛)を手駒に変換するテーブル
constexpr Hand PIECE_TO_HAND[PIECE_HAND_NB] = {
	(Hand)0,
	(Hand)(1 << PIECE_BITS[PAWN  ]) /*歩*/,
	(Hand)(1 << PIECE_BITS[LANCE ]) /*香*/,
	(Hand)(1 << PIECE_BITS[KNIGHT]) /*桂*/,
	(Hand)(1 << PIECE_BITS[SILVER]) /*銀*/,
	(Hand)(1 << PIECE_BITS[BISHOP]) /*角*/,
	(Hand)(1 << PIECE_BITS[ROOK  ]) /*飛*/,
	(Hand)(1 << PIECE_BITS[GOLD  ]) /*金*/
};

// その持ち駒を表現するのに必要なbit数のmask(例えば3bitなら2の3乗-1で7)
constexpr int PIECE_BIT_MASK[PIECE_HAND_NB] = { 0, 31/*歩は5bit*/, 7/*香は3bit*/, 7/*桂*/, 7/*銀*/, 3/*角*/, 3/*飛*/, 7/*金*/ };

constexpr u32 PIECE_BIT_MASK2[PIECE_HAND_NB] = {
	0,
	PIECE_BIT_MASK[PAWN  ] << PIECE_BITS[PAWN  ],
	PIECE_BIT_MASK[LANCE ] << PIECE_BITS[LANCE ],
	PIECE_BIT_MASK[KNIGHT] << PIECE_BITS[KNIGHT],
	PIECE_BIT_MASK[SILVER] << PIECE_BITS[SILVER],
	PIECE_BIT_MASK[BISHOP] << PIECE_BITS[BISHOP],
	PIECE_BIT_MASK[ROOK  ] << PIECE_BITS[ROOK  ],
	PIECE_BIT_MASK[GOLD  ] << PIECE_BITS[GOLD  ]
};

// 駒の枚数が格納されているbitが1となっているMASK。(駒種を得るときに使う)
constexpr u32 HAND_BIT_MASK =
	PIECE_BIT_MASK2[PAWN]   |
	PIECE_BIT_MASK2[LANCE]  |
	PIECE_BIT_MASK2[KNIGHT] |
	PIECE_BIT_MASK2[SILVER] |
	PIECE_BIT_MASK2[BISHOP] |
	PIECE_BIT_MASK2[ROOK]   |
	PIECE_BIT_MASK2[GOLD];

// 余らせてあるbitの集合。
constexpr u32 HAND_BORROW_MASK = (HAND_BIT_MASK << 1) & ~HAND_BIT_MASK;


// 手駒pcの枚数を返す。
// このASSERTを有効化するとconstexprにならないのでコメントアウトしておく。
// 返し値は引き算するときに符号を意識したくないのでintにしておく。
constexpr int hand_count(Hand hand, PieceType pr) { /* ASSERT_LV2(PIECE_HAND_ZERO <= pr && pr < PIECE_HAND_NB); */ return (int)(hand >> PIECE_BITS[pr]) & PIECE_BIT_MASK[pr]; }

// 手駒pcを持っているかどうかを返す。
constexpr u32 hand_exists(Hand hand, PieceType pr) { /* ASSERT_LV2(PIECE_HAND_ZERO <= pr && pr < PIECE_HAND_NB); */ return hand & PIECE_BIT_MASK2[pr]; }

// 歩以外の手駒を持っているか
constexpr u32 hand_except_pawn_exists(Hand hand) { return hand & (HAND_BIT_MASK ^ PIECE_BIT_MASK2[PAWN]); }

// 手駒にpcを1枚加える。
constexpr void add_hand(Hand &hand, PieceType pr) { hand = Hand(hand + PIECE_TO_HAND[pr]); }

// 手駒からpcを1枚減らす。
constexpr void sub_hand(Hand &hand, PieceType pr) { hand = Hand(hand - PIECE_TO_HAND[pr]); }


// 手駒h1のほうがh2より優れているか。(すべての種類の手駒がh2のそれ以上ある)
// 優等局面の判定のとき、局面のhash key(StateInfo::key() )が一致していなくて、盤面のhash key(StateInfo::board_key() )が
// 一致しているときに手駒の比較に用いるので、手駒がequalというケースは前提により除外されているから、この関数を以ってsuperiorであるという判定が出来る。
constexpr bool hand_is_equal_or_superior(Hand h1, Hand h2) { return ((h1-h2) & HAND_BORROW_MASK) == 0; }

// 手駒を表示する(USI形式ではない) デバッグ用
std::ostream& operator<<(std::ostream& os, Hand hand);


// --------------------	
// 手駒情報を直列化したもの	
// --------------------	
#if defined(LONG_EFFECT_LIBRARY)

// LONG_EFFECT_LIBRARYのmateルーチンで使用している。
// 修正してこのenumは削除すべきだが、わりと面倒なので出来ていない。

// 特定種の手駒を持っているかどうかをbitで表現するクラス	
// bit0..歩を持っているか , bit1..香 , bit2..桂 , bit3..銀 , bit4..角 , bit5..飛 , bit6..金 , bit7..玉(フラグとして用いるため)	
enum HandKind : uint32_t {
	HAND_KIND_PAWN = 1 << (PAWN - 1), HAND_KIND_LANCE = 1 << (LANCE - 1), HAND_KIND_KNIGHT = 1 << (KNIGHT - 1),
	HAND_KIND_SILVER = 1 << (SILVER - 1), HAND_KIND_BISHOP = 1 << (BISHOP - 1), HAND_KIND_ROOK = 1 << (ROOK - 1), HAND_KIND_GOLD = 1 << (GOLD - 1),
	HAND_KIND_KING = 1 << (KING - 1), HAND_KIND_ZERO = 0,
};

// Hand型からHandKind型への変換子	
// 例えば歩の枚数であれば5bitで表現できるが、011111bを加算すると1枚でもあれば桁あふれしてbit5が1になる。	
// これをPEXT32で回収するという戦略。	
static HandKind toHandKind(Hand h) { return (HandKind)PEXT32(h + HAND_BIT_MASK, HAND_BORROW_MASK); }

// 特定種類の駒を持っているかを判定する	
constexpr bool hand_exists(HandKind hk, Piece pt) { /* ASSERT_LV2(PIECE_HAND_ZERO <= pt && pt < PIECE_HAND_NB); */ return static_cast<bool>(hk & (1 << (pt - 1))); }

// 歩以外の手駒を持っているかを判定する	
constexpr bool hand_exceptPawnExists(HandKind hk) { return hk & ~HAND_KIND_PAWN; }

// 手駒の有無を表示する(USI形式ではない) デバッグ用	
//std::ostream& operator<<(std::ostream& os, HandKind hk);

#endif

// 平手の開始局面のSFEN文字列。
// 📝 Stockfishではengine.cppとuci.cppで定義されている。
extern const std::string StartSFEN;

// 局面 class。前方宣言。
class Position;

// 💡 ここにあった指し手生成に関するコードは、movegen.hに移動した。

// --------------------
//        探索
// --------------------

// 入玉ルール設定
enum EnteringKingRule
{
	EKR_NONE,            // 入玉ルールなし
	EKR_24_POINT,        // 24点法(31点以上で宣言勝ち)
	EKR_24_POINT_H,      // 24点法 , 駒落ち対応
	EKR_27_POINT,        // 27点法 = CSAルール(先手28点、後手27点)
	EKR_27_POINT_H,      // 27点法 , 駒落ち対応
	EKR_TRY_RULE,        // トライルール
	EKR_NULL,            // 未設定
};

// エンジンオプションの入玉ルールに関する文字列
extern std::vector<std::string> EKR_STRINGS;

// ekr_rulesで定義されている入玉ルール文字列をEnteringKingRule型に変換する。
extern EnteringKingRule to_entering_king_rule(const std::string& rule);


// 千日手の状態
enum RepetitionState
{
	REPETITION_NONE,     // 千日手ではない
	REPETITION_WIN,      // 連続王手の千日手による勝ち
	REPETITION_LOSE,     // 連続王手の千日手による負け
	REPETITION_DRAW,     // 連続王手ではない普通の千日手
	REPETITION_SUPERIOR, // 優等局面(盤上の駒が同じで手駒が相手より優れている)
	REPETITION_INFERIOR, // 劣等局面(盤上の駒が同じで手駒が相手より優れている)
	REPETITION_NB,
};

constexpr bool is_ok(RepetitionState rs) { return REPETITION_NONE <= rs && rs < REPETITION_NB; }

// RepetitionStateを文字列化する。PVの出力のときにUSI拡張として出力するのに用いる。
std::string to_usi_string(RepetitionState rs);

// RepetitionStateを文字列化して出力する。PVの出力のときにUSI拡張として出力するのに用いる。
std::ostream& operator<<(std::ostream& os, RepetitionState rs);

// 引き分け時のスコア
extern Value drawValueTable[REPETITION_NB][COLOR_NB];
static Value draw_value(RepetitionState rs, Color c) { /* ASSERT_LV3(is_ok(rs)); */ return drawValueTable[rs][c]; }

// --------------------
//      評価関数
// --------------------

class OptionsMap;

namespace Eval
{

// BonanzaでKKP/KPPと言うときのP(Piece)を表現する型。
/*
    📓 AVX2を用いて評価関数を最適化するときに32bitでないと困る。
		AVX2より前のCPUではこれは16bitでも構わないのだが、
 　		1) 16bitだと32bitだと思いこんでいてオーバーフローさせてしまうコードを書いてしまうことが多々あり、保守が困難。
	 　	2) ここが32bitであってもそんなに速度低下しないし、それはSSE4.2以前に限るから許容範囲。
		という2つの理由から、32bitに固定する。
*/
enum BonaPiece : int32_t;

}

// --------------------
//      UnitTest
// --------------------

// 前方宣言だけ。
// 実際に使う時は、"testcmd/unit_test.h"をincludeせよ。
// 使い方については、Position::UnitTest()などを参考にすること。
namespace Test
{
	class UnitTester;
	void UnitTest(Position& pos,std::istringstream& is);
}

// --------------------
//      Engine
// --------------------

// 思考エンジンinterface
class IEngine;

// thread管理class
class ThreadPool;

// EngineFuncRegisterで登録されたEngineのうち、priorityの一番高いエンジンを起動する。
void run_engine_entry();

} // namespace YaneuraOu

// --------------------
//  operators and macros
// --------------------

// ハッシュ関数(std::unorderd_map<Move16,u32>のようなものを使いたいため)
// ⚠ これはglobal namespaceで定義しないと駄目。
template<>
struct std::hash<YaneuraOu::Move16> {
	size_t operator()(const YaneuraOu::Move16& m16) const {
		return std::hash<YaneuraOu::u16>()(m16.to_u16());
	}
};

#include "extra/macros.h"

// TimePointの定義。💡Stockfishではmisc.hにある。
typedef std::chrono::milliseconds::rep TimePoint;
static_assert(sizeof(TimePoint) == sizeof(int64_t), "TimePoint should be 64 bits");

// 自作エンジンのEntry Point。
// 使い方は自作エンジンのサンプルであるUSER_ENGINEの user-engine/user-search.cpp を参考にすること。
extern void engine_main();


#endif // #ifndef _TYPES_H_INCLUDED
