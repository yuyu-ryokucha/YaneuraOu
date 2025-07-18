﻿#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include <deque>
#include <memory> // For std::unique_ptr

#include "bitboard.h"
#include "evaluate.h"
#include "types.h"

#if defined(EVAL_NNUE)
#include "eval/nnue/nnue_accumulator.h"
#endif

#include "extra/key128.h"
#include "extra/long_effect.h"
#include "misc.h"

namespace YaneuraOu {

  // --------------------
//     局面の情報
// --------------------

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

// StateInfoは、undo_move()で局面を戻すときに情報を元の状態に戻すのが面倒なものを詰め込んでおくための構造体。
// do_move()のときは、ブロックコピーで済むのでそこそこ高速。

struct StateInfo {

	// Copied when making a move
	// 指し手で局面を進めるときにコピーされる。

	// 遡り可能な手数(previousポインタを用いて局面を遡るときに用いる)
	int pliesFromNull;

	// Not copied when making a move (will be recomputed anyhow)
	// 指し手で局面を進めるときにコピーされない(なんにせよ再計算される)

	//  Key        key;

	// 盤面(盤上の駒)と手駒に関するhash key
	// 直接アクセスせずに、hand_key()、board_key(),key()を用いること。

	HASH_KEY board_key_;
	HASH_KEY hand_key_;

	// この局面のハッシュキー
	// ※　次の局面にdo_move()で進むときに最終的な値が設定される
	// board_key()は盤面のhash。hand_key()は手駒のhash。それぞれ加算したのがkey() 盤面のhash。
	// board_key()のほうは、手番も込み。
	
	Key key()                     const { return hash_key_to_key(hash_key());       }
	Key board_key()               const { return hash_key_to_key(board_hash_key()); }
	Key hand_key()                const { return hash_key_to_key(hand_hash_key());  }

	// HASH_KEY_BITSが128のときはKey128が返るhash key,256のときはKey256

	HASH_KEY hash_key()           const { return board_key_ + hand_key_; }
	HASH_KEY board_hash_key()     const { return board_key_            ; }
	HASH_KEY hand_hash_key()      const { return              hand_key_; }

#if defined(ENABLE_PAWN_HISTORY)
	// 歩の陣形に対するhash key
	HASH_KEY pawnKey_;
	Key      pawn_key()           const { return hash_key_to_key(pawn_hash_key()) >> 1;  }
	HASH_KEY pawn_hash_key()      const { return pawnKey_;               }
#endif

	// 現局面で手番側に対して王手をしている駒のbitboard
	Bitboard checkersBB;

	// この局面で捕獲された駒。先後の区別あり。
	// ※　次の局面にdo_move()で進むときにこの値が設定される
	Piece capturedPiece;

	// 一つ前の局面に遡るためのポインタ。
	// この値としてnullptrが設定されているケースは、
	// 1) root node
	// 2) 直前がnull move
	// のみである。
	// 評価関数を差分計算するときに、
	// 1)は、compute_eval()を呼び出して差分計算しないからprevious==nullで問題ない。
	// 2)は、このnodeのEvalSum sum(これはdo_move_null()でコピーされている)から
	//   計算出来るから問題ない。
	StateInfo* previous;

	// 動かすと手番側の王に対して空き王手になるかも知れない駒の候補
	// チェスの場合、駒がほとんどが大駒なのでこれらを動かすと必ず開き王手となる。
	// 将棋の場合、そうとも限らないので移動方向について考えなければならない。
	// color = 手番側 なら pinされている駒(動かすと開き王手になる)
	// color = 相手側 なら 両王手の候補となる駒。

	// 自玉に対して(敵駒によって)pinされている駒
	// blockersForKing[c]は、c側の玉に対するpin駒。すなわちc側,~c側、どちらの駒をも含む。
	Bitboard blockersForKing[COLOR_NB];

	// 自玉に対してpinしている(可能性のある)敵の大駒。
	// 自玉に対して上下左右方向にある敵の飛車、斜め十字方向にある敵の角、玉の前方向にある敵の香、…
	// ※ pinners[BLACK]は、BLACKの王に対して(pin駒が移動した時に)王手になる駒だから、WHITE側の駒。
	Bitboard pinners[COLOR_NB];

	// 自駒の駒種Xによって敵玉が王手となる升のbitboard
	Bitboard checkSquares[PIECE_TYPE_NB];

#if !defined(ENABLE_QUICK_DRAW)
	//  循環局面であることを示す。
	//   0    = 循環なし
	//   ply  = ply前の局面と同じ局面であることを表す。(ply > 0) 3回目までの繰り返し。
	//  -ply  = ply前の局面と同じ局面であることを示す。4回目の繰り返しに到達していることを示す。
	int repetition;

	// ※　以下の2つはやねうら王独自拡張。

	//  繰り返された回数 - 1。
	//  ※ repetition != 0の時に意味をなす。
	int repetition_times;

	//  その時の繰り返しの種類
	RepetitionState repetition_type;
#endif

	// この手番側の連続王手は何手前からやっているのか(連続王手の千日手の検出のときに必要)
	int continuousCheck[COLOR_NB];
  
	// この局面における手番側の持ち駒。優等局面の判定のために必要。
	Hand hand;

	// --- evaluate
#if defined(USE_CLASSIC_EVAL)

#if defined (USE_PIECE_VALUE)
	// この局面での評価関数の駒割
	Value materialValue;
#endif

#if defined(EVAL_KPPT) || defined(EVAL_KPP_KKPT)

	// 評価値。(次の局面で評価値を差分計算するときに用いる)
	// まだ計算されていなければsum.p[2][0]の値はint_max
	Eval::EvalSum sum;

#endif

#if defined(EVAL_NNUE)
	Eval::NNUE::Accumulator accumulator;
#endif

#if defined (USE_EVAL_LIST)
	// 評価値の差分計算の管理用
	Eval::DirtyPiece dirtyPiece;
#endif

#if defined(KEEP_LAST_MOVE)
	// 直前の指し手。デバッグ時などにおいてその局面までの手順を表示出来ると便利なことがあるのでそのための機能
	Move lastMove;

	// lastMoveで移動させた駒(先後の区別なし)
	PieceType lastMovedPieceType;
#endif

#endif
};

// --------------------
//     局面の定数
// --------------------

/// A list to keep track of the position states along the setup moves (from the
/// start position to the position just before the search starts). Needed by
/// 'draw by repetition' detection. Use a std::deque because pointers to
/// elements are not invalidated upon list resizing.

// setup moves("position"コマンドで設定される、現局面までの指し手)に沿った局面の状態を追跡するためのStateInfoのlist。
// 千日手の判定のためにこれが必要。std::dequeを使っているのは、StateInfoがポインターを内包しているので、resizeに対して
// 無効化されないように。
using StateList    = std::deque<StateInfo>;
using StateListPtr = std::unique_ptr<StateList>;

// --------------------
//       盤面
// --------------------

#if defined(USE_SFEN_PACKER)

// packされたsfen
struct PackedSfen {
	u8 data[32];

	// 手番を返す。
	Color color() const {
		// これは、data[0]のbit0に格納されていることは保証されている。
		return Color(data[0] & 1);
	}

	// std::unordered_mapで使用できるように==と!=を定義しておく。

	bool operator==(const PackedSfen& rhs) const {
		static_assert(sizeof(PackedSfen) % sizeof(u64) == 0);

		for (size_t i = 0; i < sizeof(PackedSfen); i += sizeof(size_t))
		{
			// 8バイト単位で比較していく。一度でも内容が異なれば不一致。
			if (*(u64*)&data[i] != *(u64*)&rhs.data[i])
				return false;
		}
		return true;
	}

	bool operator!=(const PackedSfen& rhs) const {
		return !(this->operator==(rhs));
	}
};

// std::unordered_mapで使用できるようにhash関数を定義しておく。
// std::unordered_map<PackedSfen,int,PackedSfenHash> packed_sfen_to_int;のようにtemplateの第3引数に指定する。
struct PackedSfenHash {
	size_t operator()(const PackedSfen& ps) const
	{
		static_assert(sizeof(PackedSfen) % sizeof(size_t) == 0);

		size_t s = 0;
		for (size_t i = 0; i < sizeof(PackedSfen) ; i+= sizeof(size_t))
		{
			// size_tのsize分ずつをxorしていき、それをhash keyとする。
			s ^= *(size_t*)&ps.data[i];
		}
		return s;
	}
};
#endif

// 盤面
class Position
{
public:
	// --- ctor

	// Positionのコンストラクタで平手に初期化すると、compute_eval()が呼び出され、このときに
	// 評価関数テーブルを参照するが、isready()が呼び出されていないのでこの初期化が出来ない。
	Position()                           = default;
	Position(const Position&)            = delete;
	Position& operator=(const Position&) = delete;

	// Positionで用いるZobristテーブルの初期化
	static void init();

	// sfen文字列で盤面を設定する
	// ※　内部的にinit()は呼び出される。
	// 局面を遡るために、rootまでの局面の情報が必要であるから、それを引数のsiで渡してやる。
	// 遡る必要がない場合は、StateInfo si;に対して&siなどとして渡しておけば良い。
	// 内部的にmemset(si,0,sizeof(StateInfo))として、この渡されたインスタンスをクリアしている。
#if STOCKFISH
	Position& set(const std::string& sfenStr,bool isChess960, StateInfo* si);
#else
    Position& set(const std::string& sfenStr, StateInfo* si);
#endif

	// 局面のsfen文字列を取得する
	// ※ USIプロトコルにおいては不要な機能ではあるが、デバッグのために局面を標準出力に出力して
	// 　その局面から開始させたりしたいときに、sfenで現在の局面を出力出来ないと困るので用意してある。
	// 引数としてintを取るほうのsfen()は、出力するsfen文字列の末尾の手数を指定できるバージョン。
	// ※　裏技 : gamePlyが負なら、sfen文字列末尾の手数を出力しない。
	const std::string sfen() const { return sfen(game_ply()); }
	const std::string sfen(int gamePly) const;

	// sfen()のflip(先後反転 = 盤面を180度回転)させた時のsfenを返す。
	const std::string flipped_sfen() const { return flipped_sfen(game_ply()); }
	const std::string flipped_sfen(int gamePly) const;

	// sfen文字列をflip(先後反転)したsfen文字列に変換する。
	static const std::string sfen_to_flipped_sfen(std::string sfen);

	// 平手の初期盤面を設定する。
	// siについては、上記のset()にある説明を読むこと。
	void set_hirate(StateInfo*si) { set(StartSFEN, si); }

	// --- properties

	// 現局面の手番を返す。
	Color side_to_move() const { return sideToMove; }

	// (将棋の)開始局面からの手数を返す。
	// 平手の開始局面なら1が返る。(0ではない)
	int game_ply() const { return gamePly; }

	// 盤面上の駒を返す。
	// ※ sq == SQ_NBの時、NO_PIECEが返ることは保証されている。
	Piece piece_on(Square sq) const { ASSERT_LV3(sq <= SQ_NB); return board[sq]; }

	// ある升に駒がないならtrueを返す。
	bool empty(Square sq) const { return piece_on(sq) == NO_PIECE; }

	// c側の手駒を返す。
	Hand hand_of(Color c) const { ASSERT_LV3(is_ok(c));  return hand[c]; }

	// ↑のtemplate版
	template <Color C>
	Hand hand_of() const { ASSERT_LV3(is_ok(C));  return hand[C]; }

	// c側の玉の位置を返す。
	// Stockfishには
	//   template<PieceType Pt> Square square(Color c) const
	// というmethodがあるが、PtにはKINGしか渡せないので要らないと思う。
	FORCE_INLINE Square king_square(Color c) const { ASSERT_LV3(is_ok(c)); return kingSquare[c]; }

	// ↑のtemplate版
	template <Color C>
	Square king_square() const { ASSERT_LV3(is_ok(C)); return kingSquare[C]; }

	// 保持しているデータに矛盾がないかテストする。
	bool pos_is_ok() const;

	// 現局面に対して
	// この指し手によって移動させる駒を返す。(移動前の駒)
	// 駒打ちに対しては、その打つ駒が返る。
	// また、後手の駒打ちは後手の(その打つ)駒が返る。
	Piece moved_piece_before(Move m) const
	{
		ASSERT_LV3(m.is_ok());
#if defined( KEEP_PIECE_IN_GENERATE_MOVES)
		// 上位16bitに格納されている値を利用する。
		// return is_promote(m) ? (piece & ~PIECE_PROMOTE) : piece;
		// みたいなコードにしたいので、
		// mのMOVE_PROMOTEのbitで、PIECE_PROMOTEのbitを反転させてやる。
		static_assert(MOVE_PROMOTE == (1 << 15) && PIECE_PROMOTE == 8, "");
		return (Piece)((m ^ ((m & MOVE_PROMOTE) << 4)) >> 16);

#else
		return m.is_drop() ? make_piece(sideToMove , m.move_dropped_piece()) : piece_on(m.from_sq());
#endif
	}

	// moved_pieceの拡張版。
	// 成りの指し手のときは成りの指し手を返す。(移動後の駒)
	// Moveの上位16bitにそれが格納されているので、単にそれを返しているだけ。
	Piece moved_piece_after(Move m) const
	{
		// ASSERT_LV3(is_ok(m));
		// ⇨ MovePicker から Move::none()に対してこの関数が呼び出されることがあるのでこのASSERTは書けない。

		return m.moved_after_piece();
	}

	// 指し手mで移動させた駒(成りの指し手である場合は、成った後の駒)
    // 💡 Stockfishの探索部で用いているので、それと互換性を保つために用意。
    // 📝 Stockfishでは移動させた駒(moved_piece_before())を期待しているが、
	//     moved_piece_after()にしたほうが強いっぽいので、やねうら王では
	//     moved_piece()は、moved_piece_after()のaliasとする。
	Piece moved_piece(Move m) const { return moved_piece_after(m); }

	// 定跡DBや置換表から取り出したMove16(16bit型の指し手)を32bit化する。
	// is_ok(m) == false ならば、mをそのまま返す。
	// 例 : MOVE_WINやMOVE_NULLに対してはそれがそのまま返ってくる。つまり、この時、上位16bitは0(NO_PIECE)である。
	//
	// ※　このPositionクラスが保持している現在の手番(side_to_move)が移動させる駒に反映される。
	// ※　mの移動元の駒が現在の手番の駒でなければ、MOVE_NONEが返ることは保証される。
	// ※  mの移動元に駒がない場合も、MOVE_NONEが返ることは保証される。
	Move to_move(Move16 m) const;

	// 1. ENABLE_QUICK_DRAWがdefineされている時
	//		この関数は無視される。
	//
	// 2. ENABLE_QUICK_DRAWがdefineされていない時
	// 　　is_repetition() , has_repeted()で最大で何手前からの千日手をチェックするか。デフォルト16手。
	// 
	// ※　これを MAX_PLY に設定すると初手からのチェックになるが、将棋はチェスと異なり
	// 　　終局までの平均手数がわりと長いので、そこまでするとスピードダウンしてR40ほど弱くなる。
	void set_max_repetition_ply(int ply){ max_repetition_ply = ply;}

	// 普通の千日手、連続王手の千日手等を判定する。
	// そこまでの局面と同一局面であるかを、局面を遡って調べる。
	// 
	// 1. ENABLE_QUICK_DRAWがdefineされている時(大会用に少しでも強くしたい時)
	// plyは無視される。遡る手数は16手固定。
	//
	// 2. ENABLE_QUICK_DRAWがdefineされていない時(正確に千日手の判定を行いたい時)
	// 遡る手数は、set_max_repetition_ply()で設定された手数だけ遡る。
	// ply         : rootからの手数。3回目の同一局面の出現まではrootよりは遡って千日手と判定しない。4回目は判定する。
	RepetitionState is_repetition(int ply = 16) const;

	// is_repetition()の、千日手が見つかった時に、現局面から何手遡ったかを返すバージョン。
	// REPETITION_NONEではない時は、found_plyにその値が返ってくる。	// ※　定跡生成の時にしか使わない。
	RepetitionState is_repetition(int ply , int& found_ply) const;

#if !defined(ENABLE_QUICK_DRAW)
	// Tests whether there has been at least one repetition
	// of positions since the last capture or pawn move.
	bool has_repeated() const;
#endif

	// --- Bitboard

	// c == BLACK : 先手の駒があるBitboardが返る
	// c == WHITE : 後手の駒があるBitboardが返る
	Bitboard pieces(Color c) const;
	
	// ↑のtemplate版
	template<Color C>
	Bitboard pieces() const { ASSERT_LV3(is_ok(C)); return byColorBB[C]; }

	// 駒に対応するBitboardを得る。
	// ・引数でcの指定がないものは先後両方の駒が返る。
	// ・引数がPieceTypeのものは、PieceTypeのPAWN～DRAGON 以外に
	//		PieceTypeの GOLDS(金相当の駒) , HDK(馬・龍・玉) , BISHOP_HORSE(角・馬) , ROOK_DRAGON(飛車・龍)などが指定できる。
	//   ※　詳しくは、PieceTypeの定義を見ること。
	// ・引数でPieceTypeを複数取るものはそれらの駒のBitboardを合成したものが返る。
	// ・pr として ALL_PIECESを指定した場合、先手か後手か、いずれかの駒がある場所が1であるBitboardが返る。

	Bitboard pieces(PieceType pr = ALL_PIECES) const { ASSERT_LV3(pr < PIECE_BB_NB); return byTypeBB[pr]; }
	template<typename ...PieceTypes> Bitboard pieces(PieceType pt, PieceTypes... pts) const;

	// ↑のtemplate版
	template <PieceType PR>
	Bitboard pieces() const { ASSERT_LV3(PR < PIECE_BB_NB); return byTypeBB[PR]; }
	template<typename ...PieceTypes> Bitboard pieces(Color c, PieceTypes... pts) const;

	// ↑のtemplate版
	template<Color C>
	Bitboard pieces(PieceType pr) const { return pieces(pr) & pieces(C); }
	template<Color C,PieceType PR>
	Bitboard pieces() const { return pieces(PR) & pieces(C); }

	// 駒がない升が1になっているBitboardが返る
	Bitboard empties() const { return pieces() ^ Bitboard(1); }

	// --- 王手

	// 現局面で王手している駒
	Bitboard checkers() const { return st->checkersBB; }

	// c側の玉に対してpinしている駒(その駒をc側の玉との直線上から動かしたときにc側の玉に王手となる)
	Bitboard blockers_for_king(Color c) const { return st->blockersForKing[c]; }

	// ↑のtemplate版
	template <Color C>
	Bitboard blockers_for_king() const { return st->blockersForKing[C]; }

	// 現局面で駒Ptを動かしたときに王手となる升を表現するBitboard
	Bitboard check_squares(PieceType pt) const { ASSERT_LV3(pt!= NO_PIECE_TYPE && pt < PIECE_TYPE_NB); return st->checkSquares[pt]; }

	// c側の玉に対してpinしている駒
	// ※ pinされているではなく、pinしているということに注意。
	// 　すなわち、pinされている駒が移動した時に、この大駒によって王が素抜きにあう。
	Bitboard pinners(Color c) const { return st->pinners[c]; }

	// c側の玉に対して、指し手mが空き王手となるのか。
	bool is_discovery_check_on_king(Color c, Move m) const { return st->blockersForKing[c] & m.from_sq(); }

	// --- 利き

	// sqに利きのあるc側の駒を列挙する。cの指定がないものは先後両方の駒が返る。
	// occが指定されていなければ現在の盤面において。occが指定されていればそれをoccupied bitboardとして。
	// sq == SQ_NBでの呼び出しは合法。Bitboard(ZERO)が返る。

	Bitboard attackers_to(Color c, Square sq) const { return c==BLACK ? attackers_to<BLACK>(sq, pieces()): attackers_to<WHITE>(sq, pieces()); }
	Bitboard attackers_to(Color c, Square sq, const Bitboard& occ) const { return c==BLACK ? attackers_to<BLACK>(sq, occ): attackers_to<WHITE>(sq, occ); }
	Bitboard attackers_to(Square sq) const { return attackers_to(sq, pieces()); }
	Bitboard attackers_to(Square sq, const Bitboard& occ) const;

	template <Color C>
	Bitboard attackers_to(Square sq) const { return attackers_to<C>(sq, pieces()); }

	template <Color C>
	Bitboard attackers_to(Square sq, const Bitboard& occ) const;

	// 打ち歩詰め判定に使う。王に打ち歩された歩の升をpawn_sqとして、c側(王側)のpawn_sqへ利いている駒を列挙する。香が利いていないことは自明。
	Bitboard attackers_to_pawn(Color c, Square pawn_sq) const;

	// attackers_to()で駒があればtrueを返す版。(利きの情報を持っているなら、軽い実装に変更できる)
	// kingSqの地点からは玉を取り除いての利きの判定を行なう。
#if !defined(LONG_EFFECT_LIBRARY)
	bool effected_to(Color c, Square sq) const { return attackers_to(c, sq, pieces()); }
	bool effected_to(Color c, Square sq, Square kingSq) const { return attackers_to(c, sq, pieces() ^ kingSq); }
#else 
	bool effected_to(Color c, Square sq) const { return board_effect[c].effect(sq) != 0; }
	bool effected_to(Color c, Square sq, Square kingSq) const {
		return board_effect[c].effect(sq) != 0 ||
			((long_effect.directions_of(c, kingSq) & Effect8::directions_of(kingSq, sq)) != 0); // 影の利きがある
	}
#endif


	/// update_slider_blockers() calculates st->blockersForKing[c] and st->pinners[~c],
	/// which store respectively the pieces preventing king of color c from being in check
	/// and the slider pieces of color ~c pinning pieces of color c to the king.

	// update_slider_blockers()はst->blockersForKing[c]およびst->pinners[~c]を計算します。
	// これらはそれぞれ、色cの王が王手状態になるのを防ぐ駒と、色cの駒を王にピン留めする手番~cの
	// スライダー駒を格納しています。
	// ※　「ピン留め」とは、移動させた時に開き王手となること。
	// 
	// 注意)
	// 	 王 歩 ^飛 ^飛
	//  のようなケースにおいては、この両方の飛車がpinnersとして列挙される。(SEEの処理でこういう列挙がなされて欲しいので)

	void update_slider_blockers(Color c) const;

	// c側の駒Ptの利きのある升を表現するBitboardを返す。(MovePickerで用いている。)
	template<Color C , PieceType Pt> Bitboard attacks_by() const;

	// --- 局面を進める/戻す

	// 指し手で盤面を1手進める
	// m = 指し手。mとして非合法手を渡してはならない。
	// info = StateInfo。局面を進めるときに捕獲した駒などを保存しておくためのバッファ。
	// このバッファはこのdo_move()の呼び出し元の責任において確保されている必要がある。
	// givesCheck = mの指し手によって王手になるかどうか。
	// この呼出までにst.checkInfo.update(pos)が呼び出されている必要がある。
	void do_move(Move m, StateInfo& newSt, bool givesCheck);

	// do_move()の4パラメーター版のほうを呼び出すにはgivesCheckも渡さないといけないが、
	// mで王手になるかどうかがわからないときはこちらの関数を用いる。
	void do_move(Move m, StateInfo& newSt) { do_move(m, newSt, gives_check(m)); }

	// 指し手で盤面を1手戻す
	void undo_move(Move m);

	// null move用のdo_move()
	void do_null_move(StateInfo& st);
	// null move用のundo_move()
	void undo_null_move();

	// --- legality(指し手の合法性)のチェック

	// 生成した指し手(CAPTUREとかNON_CAPTUREとか)が、合法であるかどうかをテストする。
	//
	// 指し手生成で合法手であるか判定が漏れている項目についてチェックする。
	// 王手のかかっている局面についてはEVASION(回避手)で指し手が生成されているはずなので
	// ここでは王手のかかっていない局面における合法性のチェック。
	// 具体的には、
	//  1) 移動させたときに素抜きに合わないか
	//  2) 敵の利きのある場所への王の移動でないか
	// ※　連続王手の千日手などについては探索の問題なのでこの関数のなかでは行わない。
	// ※　それ以上のテストは行わないので、置換表から取ってきた指し手などについては、
	// pseudo_legal()を用いて、そのあとこの関数で判定すること。
	// 歩の不成に関しては、この関数は常にtrueを返す。(合法扱い)
	bool legal(Move m) const;

	// mがpseudo_legalな指し手であるかを判定する。
    /*
	    📓　pseudo_legalとは

			pseudo_legalとは擬似合法手のこと。ここには、自殺手が含まれている。

			置換表の指し手でdo_move()して良いのかの事前判定のために使われる。
			指し手生成ルーチンのテストなどにも使える。(指し手生成ルーチンはpseudo_legalな指し手を返すはずなので)

			killerのような兄弟局面の指し手がこの局面において合法かどうかにも使う。
			※　置換表の検査だが、pseudo_legal()で擬似合法手かどうかを判定したあとlegal()で自殺手でないことを
			確認しなくてはならない。このためpseudo_legal()とlegal()とで重複する自殺手チェックはしていない。

			is_ok(m)==falseの時、すなわち、m == MOVE_WINやMOVE_NONEのような時に
			Position::to_move(m) == mは保証されており、この時、本関数pseudo_legal(m)がfalseを返すことは保証する。
			generate_all_legal_moves : これがtrueならば、歩の不成も合法手扱い。

		⚠ 常に歩の不成の指し手も合法手として扱いたいならば、
			この関数ではなく、pseudo_legal_s<true>()を用いること。
	*/
    bool pseudo_legal(const Move m, bool generate_all_legal_moves) const;

	// All == false        : 歩や大駒の不成に対してはfalseを返すpseudo_legal()
	template <bool All> bool pseudo_legal_s(const Move m) const;

	// toの地点に歩を打ったときに打ち歩詰めにならないならtrue。
	// 歩をtoに打つことと、二歩でないこと、toの前に敵玉がいることまでは確定しているものとする。
	// 二歩の判定もしたいなら、legal_pawn_drop()のほうを使ったほうがいい。
	bool legal_drop(const Square to) const;

	// 二歩でなく、かつ打ち歩詰めでないならtrueを返す。
	bool legal_pawn_drop(const Color us, const Square to) const;

	// leagl()では、成れるかどうかのチェックをしていない。
	// (先手の指し手を後手の指し手と混同しない限り、指し手生成された段階で
	// 成れるという条件は満たしているはずだから)
	// しかし、先手の指し手を後手の指し手と取り違えた場合、この前提が崩れるので
	// これをチェックするための関数。成れる条件を満たしていない場合、falseが返る。
	bool legal_promote(Move m) const;

	// --- StateInfo

	// 現在の局面に対応するStateInfoを返す。
	// たとえば、state()->capturedPieceであれば、前局面で捕獲された駒が格納されている。
	StateInfo* state() const { return st; }

	// --- Evaluation

#if defined(USE_EVAL_LIST)
	// 評価関数で使うための、どの駒番号の駒がどこにあるかなどの情報。
	const Eval::EvalList* eval_list() const { return &evalList; }
#endif

#if defined (USE_SEE)
	// 指し手mのsee(Static Exchange Evaluation : 静的取り合い評価)において
	// v(しきい値)以上になるかどうかを返す。
	// see_geのgeはgreater or equal(「以上」の意味)の略。
	bool see_ge(Move m, Value threshold = VALUE_ZERO) const;

#endif

	// --- Accessing hash keys

	// StateInfo::key()への簡易アクセス。
	Key           key() const { return st->key()     ; }
	HASH_KEY hash_key() const { return st->hash_key(); }

	// ある指し手を指した後のhash keyを返す。
	// 将棋だとこの計算にそこそこ時間がかかるので、通常の探索部でprefetch用に
	// これを計算するのはあまり得策ではないが、詰将棋ルーチンでは置換表を投機的に
	// prefetchできるとずいぶん速くなるのでこの関数を用意しておく。
	Key      key_after     (Move m) const;
	HASH_KEY hash_key_after(Move m) const;

#if defined(ENABLE_PAWN_HISTORY)
	// 歩の陣形に対するhash key
	// やねうら王ではbit0を手番に用いているので、ここを使わないように >> 1して値を返す。
	Key pawn_key() const { return st->pawn_key() >> 1; }
#endif

	// --- misc

	// 現局面で王手がかかっているか
	bool in_check() const { return checkers(); }

	// ピンされているc側の駒。下手な方向に移動させるとc側の玉が素抜かれる。
	// 手番側のpinされている駒はpos.pinned_pieces(pos.side_to_move())のようにして取得できる。
	// LONG_EFFECT_LIBRARYを使うときのmateルーチンで使用しているので消さないで！
	Bitboard pinned_pieces(Color c) const { ASSERT_LV3(is_ok(c)); return st->blockersForKing[c] & pieces(c); }

	// ↑のtemplate版
	template<Color C>
	Bitboard pinned_pieces() const { ASSERT_LV3(is_ok(C)); return st->blockersForKing[C] & pieces<C>(); }

	// avoidで指定されている遠方駒は除外して、pinされている駒のbitboardを得る。
	// ※利きのない1手詰め判定のときに必要。
	Bitboard pinned_pieces(Color c, Square avoid) const { return c == BLACK ? pinned_pieces<BLACK>(avoid) : pinned_pieces<WHITE>(avoid); }

	template<Color C>
	Bitboard pinned_pieces(Square avoid) const;

	// fromからtoに駒が移動したものと仮定して、pinを得る
	// ※利きのない1手詰め判定のときに必要。
	Bitboard pinned_pieces(Color c, Square from, Square to) const
	{
		return c == BLACK ? pinned_pieces<BLACK>(from,to) : pinned_pieces<WHITE>(from,to);
	}

	// ↑のtemplate版
	template<Color C>
	Bitboard pinned_pieces(Square from, Square to) const;


	// 指し手mで王手になるかを判定する。
	// 前提条件 : 指し手mはpseudo-legal(擬似合法)の指し手であるものとする。
	// (つまり、mのfromにある駒は自駒であることは確定しているものとする。)
	bool gives_check(Move m) const;

	// 手番側の駒をfromからtoに移動させると素抜きに遭うのか？
	bool discovered(Square from, Square to, Square ourKing, const Bitboard& pinned) const
	{
		// 1) pinされている駒がないなら素抜きにならない。
		// 2) pinされている駒でなければ素抜き対象ではない
		// 3) pinされている駒でも王と(縦横斜において)直線上への移動であれば合法
		return pinned                        // 1)
			&& (pinned & from)               // 2)
			&& !aligned(from, to , ourKing); // 3)
	}

	// 現局面で指し手がないかをテストする。指し手生成ルーチンを用いるので速くない。探索中には使わないこと。
	bool is_mated() const;

	// 直前の指し手によって捕獲した駒。先後の区別あり。
	Piece captured_piece() const { return st->capturedPiece; }

	// 捕獲する指し手か、成りの指し手であるかを判定する。
	bool capture_or_promotion(Move m) const { return m.is_promote() || capture(m); }

	// 歩の成る指し手であるか？
	bool pawn_promotion(Move m) const
	{
		// 移動させる駒が歩かどうかは、Moveの上位16bitを見れば良い
		return (m.is_promote() && raw_type_of(moved_piece_after(m)) == PAWN);
	}

	// 捕獲する指し手か、歩の成りの指し手であるかを返す。
	bool capture_or_pawn_promotion(Move m) const
	{
		return pawn_promotion(m) || capture(m);
	}

	// 捕獲か価値のある駒の成り。(歩、角、飛車)
	bool capture_or_valuable_promotion(Move m) const
	{
		// 歩の成りを角・飛車の成りにまで拡大する。
		auto pr = raw_type_of(moved_piece_after(m));
		return (m.is_promote() && (pr == PAWN || pr == BISHOP || pr == ROOK)) || capture(m);
	}

	// 捕獲する指し手であるか。
	bool capture(Move m) const { return !m.is_drop() && piece_on(m.to_sq()) != NO_PIECE; }


	// Stockfishにはcapture_stage()というメソッドが追加された。下記のコード。
	// これは、捕獲する指し手かQUEENにpromoteする指し手かのどちらかであるかを判定する。
	// 将棋で言うとcapture_or_valuable_promotion()みたいなもの。

	//// returns true if a move is generated from the capture stage
	//// having also queen promotions covered, i.e. consistency with the capture stage move generation
	//// is needed to avoid the generation of duplicate moves.
	//bool capture_stage(Move m) const {
	//  assert(is_ok(m));
	//  return  capture(m) || promotion_type(m) == QUEEN;
	//}

	// →　互換性維持のために、capture_stageを定義。
	bool capture_stage(Move m) const
	{
		//return capture_or_valuable_promotion(m);
		//return capture_or_pawn_promotion(m);

		// →　V7.73y3とy4,y5の比較。
		// 単にcapture()にするのが一番良かった。

		return capture(m);
	}

	// 入玉時の宣言勝ち
    /*
		📓 宣言勝ちの条件を満たしているとき、MOVE_WINや、玉を移動する指し手(トライルール時)が返る。
			さもなくば、MOVE_NONEが返る。

		⚠ 事前にset_ekr()で入玉ルールが設定されていること。
	*/
	Move DeclarationWin() const;

	// 入玉の宣言勝ちのルールを設定する。このルールに基づいて入玉の計算が行われる。
	// 現在の盤面を見て、平手から足りない駒を計算するので、現在の盤面は適切に設定されている必要がある。
	// 📝 start_searching()のなかで設定すると良いと思う。
    void set_ekr(EnteringKingRule ekr) {
        this->ekr = ekr;
        update_entering_point();
    }

	// -- sfen化ヘルパ
#if defined(USE_SFEN_PACKER)
  // packされたsfenを得る。引数に指定したバッファに返す。
  // gamePlyはpackに含めない。
	void sfen_pack(PackedSfen& sfen);

	// packされたsfenを解凍する。sfen文字列が返る。
	// gamePly = 0となる。
	static std::string sfen_unpack(const PackedSfen& sfen);

	// ↑sfenを経由すると遅いので直接packされたsfenをセットする関数を作った。
	// pos.set(sfen_unpack(data),si); と等価。
	// 渡された局面に問題があって、エラーのときはTools::Result::SomeErrorを返す。
	// PackedSfenにgamePlyは含まないので復元できない。そこを設定したいのであれば引数で指定すること。
	Tools::Result set_from_packed_sfen(const PackedSfen& sfen , StateInfo * si , bool mirror=false , int gamePly_ = 0);

	// 盤面と手駒、手番を与えて、そのsfenを返す。
	static std::string sfen_from_rawdata(Piece board[81], Hand hands[2], Color turn, int gamePly);
#endif

	// -- 利き
#if defined(LONG_EFFECT_LIBRARY)

	// 各升の利きの数
	LongEffect::ByteBoard board_effect[COLOR_NB];

	// NNUE halfKPE9で局面の差分計算をするときに用いる
#if defined(USE_BOARD_EFFECT_PREV)
	// 前局面のboard_effect（評価値の差分計算用）

	// 構造的には、StateInfoが持つべきなのだが、探索のほうで
	// do_move()して次のnodeのsearch()が呼び出された直後にしかevaluate()は
	// 呼び出さないので、do_move()でこの利きをboard_effectからコピーすれば
	// KPE9の差分計算で困ることはない。
	LongEffect::ByteBoard board_effect_prev[COLOR_NB];
#endif

	// 長い利き(これは先後共用)
	LongEffect::WordBoard long_effect;

#endif

	// --- デバッグ用の出力

#if defined(KEEP_LAST_MOVE)
  // 開始局面からこの局面にいたるまでの指し手を表示する。
	std::string moves_from_start() const { return moves_from_start(false); }
	std::string moves_from_start_pretty() const { return moves_from_start(true); }
	std::string moves_from_start(bool is_pretty) const;
#endif

	// 盤面を出力する。(USI形式ではない) デバッグ用。
	friend std::ostream& operator<<(std::ostream& os, const Position& pos);

	// MoveGenerator(指し手生成器)からは盤面・手駒にアクセス出来る必要があるのでfriend
	template <MOVE_GEN_TYPE gen_type, bool gen_all>
	friend struct MoveGenerator;

	// UnitTest
	static void UnitTest(Test::UnitTester& tester, IEngine& engine);

private:

	/// Position::set_state() computes the hash keys of the position, and other
	/// data that once computed is updated incrementally as moves are made.
	/// The function is only used when a new position is set up

	// Position::set_state()は、局面のハッシュキーおよび、
	// 一度計算されると手が指されるたびに差分更新されるその他のデータを計算します。
	// この関数は、新しい局面が設定されたときのみ使用されます。

	void set_state() const;

	// 王手になるbitboard等を更新する。set_state()とdo_move()のときに自動的に行われる。
	// null moveのときは利きの更新を少し端折れるのでフラグを渡すことに。
	template <bool doNullMove,Color Us>
	void set_check_info() const;

	template <bool doNullMove>
	void set_check_info() const
	{
		sideToMove == BLACK ? set_check_info<doNullMove, BLACK>() : set_check_info<doNullMove, WHITE>();
	}

	// do_move()の先後分けたもの。内部的に呼び出される。
	template <Color Us> void do_move_impl(Move m, StateInfo& st, bool givesCheck);

	// undo_move()の先後分けたもの。内部的に呼び出される。
	template <Color Us> void undo_move_impl(Move m);

	// 📝 update_entering_point()は、現在の盤面から、事前にset_ekr_ruleで設定されたルールに基づき、
	//     入玉に必要な駒点を計算し、enteringKingPoint[]に設定する。
	//     ここで設定された値が、DeclarationWin()の時に用いられる。

	void update_entering_point();
    EnteringKingRule ekr = EKR_NULL;
	int enteringKingPoint[COLOR_NB];

	// --- Bitboards
	// alignas(16)を要求するものを先に宣言。

	// 盤上の先手/後手/両方の駒があるところが1であるBitboard
	Bitboard byColorBB[COLOR_NB];

	// 駒が存在する升を表すBitboard。先後混在。
	// pieces()の引数と同じく、ALL_PIECES,HDKなどのPieceで定義されている特殊な定数が使える。
	Bitboard byTypeBB[PIECE_BB_NB];

	// put_piece()やremove_piece()、xor_piece()を用いたときは、最後にupdate_bitboards()を呼び出して
	// bitboardの整合性を保つこと。
	// また、put_piece_simple()は、put_piece()の王の升(kingSquare)を更新しない版。do_move()で用いる。

	// 駒を配置して、内部的に保持しているBitboardなどを更新する。
	// 注意1 : kingを配置したときには、このクラスのkingSqaure[]を更新しないといけないが、
	// この関数のなかでは行っていないので呼び出し側で更新すること。
	// 注意2 : evalListのほうの更新もこの関数のなかでは行っていないので必要ならば呼び出し側で更新すること。
	// 例) 
	// if (type_of(pc) == KING)
	//		kingSquare[color_of(pc)] = sq;
	// もしくはupdate_kingSquare()を呼び出すこと。
	void put_piece(Square sq, Piece pc);

	// 駒を盤面から取り除き、内部的に保持しているBitboardも更新する。
	void remove_piece(Square sq);

	// sqの地点にpcを置く/取り除く、したとして内部で保持しているBitboardを更新する。
	// 最後にupdate_bitboards()を呼び出すこと。
	void xor_piece(Piece pc, Square sq);

	// put_piece(),remove_piece(),xor_piece()を用いたあとに呼び出す必要がある。
	void update_bitboards();

	// このクラスが保持しているkingSquare[]の更新。
	// put_piece(),remove_piece(),xor_piece()では玉の位置(kingSquare[])を
	// 更新してくれないので、自前で更新するか、一連の処理のあとにこの関数を呼び出す必要がある。
	void update_kingSquare();

#if defined (USE_EVAL_LIST)
	// --- 盤面を更新するときにEvalListの更新のために必要なヘルパー関数

	// c側の手駒ptの最後の1枚のBonaPiece番号を返す
	Eval::BonaPiece bona_piece_of(Color c, PieceType pt) const {
		// c側の手駒ptの枚数
		int ct = hand_count(hand[c], pt);
		ASSERT_LV3(ct > 0);
		return (Eval::BonaPiece)(Eval::kpp_hand_index[c][pt].fb + ct - 1);
	}

	// c側の手駒ptの(最後の1枚の)PieceNumberを返す。
	PieceNumber piece_no_of(Color c, PieceType pt) const { return evalList.piece_no_of_hand(bona_piece_of(c, pt)); }

	// 盤上のsqの升にある駒のPieceNumberを返す。
	PieceNumber piece_no_of(Square sq) const
	{
		ASSERT_LV3(piece_on(sq) != NO_PIECE);
		PieceNumber n = evalList.piece_no_of_board(sq);
		ASSERT_LV3(is_ok(n));
		return n;
	}
#else
	// 駒番号を使わないとき用のダミー
	PieceNumber piece_no_of(Color c, Piece pt) const { return PIECE_NUMBER_ZERO; }
	PieceNumber piece_no_of(Piece pc, Square sq) const { return PIECE_NUMBER_ZERO; }
	PieceNumber piece_no_of(Square sq) const { return PIECE_NUMBER_ZERO; }
#endif
	// ---

	// 盤面、81升分の駒 + 1
	Piece board[SQ_NB_PLUS1];

	// 手駒
	Hand hand[COLOR_NB];

	// 手番
	Color sideToMove;

	// 玉の位置
	Square kingSquare[COLOR_NB];

	// 初期局面からの手数(初期局面 == 1)
	int gamePly;

	// 現局面に対応するStateInfoのポインタ。
	// do_move()で次の局面に進むときは次の局面のStateInfoへの参照をdo_move()の引数として渡される。
	//   このとき、undo_move()で戻れるようにStateInfo::previousに前のstの値を設定しておく。
	// undo_move()で前の局面に戻るときはStateInfo::previousから辿って戻る。
	StateInfo* st;

	// set_max_repetition_ply()で設定される、千日手の最大遡り手数
	static int max_repetition_ply /* = 16 */;

#if defined(USE_EVAL_LIST)
	// 評価関数で用いる駒のリスト
	Eval::EvalList evalList;
#endif
};

template<typename ...PieceTypes>
inline Bitboard Position::pieces(PieceType pt, PieceTypes... pts) const {
  return pieces(pt) | pieces(pts...);
}

inline Bitboard Position::pieces(Color c) const {
  return byColorBB[c];
}

template<typename ...PieceTypes>
inline Bitboard Position::pieces(Color c, PieceTypes... pts) const {
  return pieces(c) & pieces(pts...);
}

// sに利きのあるc側の駒を列挙する。
// (occが指定されていなければ現在の盤面において。occが指定されていればそれをoccupied bitboardとして)
template <Color C>
inline Bitboard Position::attackers_to(Square sq, const Bitboard& occ) const
{
	ASSERT_LV3(is_ok(C) && sq <= SQ_NB);

	constexpr Color Them = ~C;

	// sの地点に敵駒ptをおいて、その利きに自駒のptがあればsに利いているということだ。
	// 香の利きを求めるコストが惜しいのでrookEffect()を利用する。
	return
		(     (pawnEffect  <Them>(sq)	&  pieces(PAWN)        )
			| (knightEffect<Them>(sq)	&  pieces(KNIGHT)      )
			| (silverEffect<Them>(sq)	&  pieces(SILVER_HDK)  )
			| (goldEffect  <Them>(sq)	&  pieces(GOLDS_HDK)   )
			| (bishopEffect(sq, occ)	&  pieces(BISHOP_HORSE))
			| (rookEffect(sq, occ)		& (pieces(ROOK_DRAGON) | (lanceStepEffect<Them>(sq) & pieces(LANCE))))
			//  | (kingEffect(sq) & pieces(c, HDK));
			// →　HDKは、銀と金のところに含めることによって、参照するテーブルを一個減らして高速化しようというAperyのアイデア。
			) & pieces<C>(); // 先後混在しているのでc側の駒だけ最後にマスクする。
	;
}

// c側の駒Ptの利きのある升を表現するBitboardを返す。(MovePickerで用いている。)
// 遠方駒に関しては盤上の駒を考慮した利き。
template<Color C , PieceType Pt>
Bitboard Position::attacks_by() const
{
	if constexpr (Pt == PAWN)
		return C == WHITE ? pawn_attacks_bb<WHITE>(pieces<WHITE, PAWN>()) : pawn_attacks_bb<BLACK>(pieces<BLACK, PAWN>());
	else
	{
		Bitboard threats   = Bitboard(ZERO);
		Bitboard attackers = pieces(C, Pt);
		while (attackers)
			threats |= attacks_bb<make_piece(C,Pt)>(attackers.pop(), pieces());
		return threats;
	}
}

// ピンされているc側の駒。下手な方向に移動させるとc側の玉が素抜かれる。
// avoidで指定されている遠方駒は除外して、pinされている駒のbitboardを得る。
template <Color C>
Bitboard Position::pinned_pieces(Square avoid) const
{
	Bitboard b, pinners, result = Bitboard(ZERO);
	Square ksq = king_square(C);

	// avoidを除外して考える。
	Bitboard avoid_bb = ~Bitboard(avoid);

	pinners = (
		(  pieces(ROOK_DRAGON)   & rookStepEffect    (ksq))
		| (pieces(BISHOP_HORSE)  & bishopStepEffect  (ksq))
		| (pieces(LANCE)         & lanceStepEffect<C>(ksq))
		) & avoid_bb & pieces<~C>();

	while (pinners)
	{
		b = between_bb(ksq, pinners.pop()) & pieces() & avoid_bb;
		if (!b.more_than_one())
			result |= b & pieces<C>();
	}
	return result;
}

// ピンされているc側の駒。下手な方向に移動させるとc側の玉が素抜かれる。
// fromからtoに駒が移動したものと仮定して、pinを得る
template <Color C>
Bitboard Position::pinned_pieces(Square from, Square to) const {
	Bitboard b, pinners, result = Bitboard(ZERO);
	Square ksq = king_square(C);

	// avoidを除外して考える。
	Bitboard avoid_bb = ~Bitboard(from);

	pinners = (
		(  pieces(ROOK_DRAGON)  & rookStepEffect    (ksq))
		| (pieces(BISHOP_HORSE) & bishopStepEffect  (ksq))
		| (pieces(LANCE)        & lanceStepEffect<C>(ksq))
		) & avoid_bb & pieces<~C>();

	// fromからは消えて、toの地点に駒が現れているものとして
	Bitboard new_pieces = (pieces() & avoid_bb) | to;
	while (pinners)
	{
		b = between_bb(ksq, pinners.pop()) & new_pieces;
		if (!b.more_than_one())
			result |= b & pieces(C);
	}
	return result;
}

inline void Position::xor_piece(Piece pc, Square sq)
{
	// 先手・後手の駒のある場所を示すoccupied bitboardの更新
	byColorBB[color_of(pc)] ^= sq;

	// 先手 or 後手の駒のある場所を示すoccupied bitboardの更新
	byTypeBB[ALL_PIECES] ^= sq;

	// 駒別のBitboardの更新
	// これ以外のBitboardの更新は、update_bitboards()で行なう。
	byTypeBB[type_of(pc)] ^= sq;
}

// 駒を配置して、内部的に保持しているBitboardも更新する。
inline void Position::put_piece(Square sq, Piece pc)
{
	ASSERT_LV2(board[sq] == NO_PIECE);
	board[sq] = pc;
	xor_piece(pc, sq);
}

// 駒を盤面から取り除き、内部的に保持しているBitboardも更新する。
inline void Position::remove_piece(Square sq)
{
	Piece pc = board[sq];
	ASSERT_LV3(pc != NO_PIECE);
	board[sq] = NO_PIECE;
	xor_piece(pc, sq);
}

inline bool is_ok(Position& pos) { return pos.pos_is_ok(); }

// 盤面を出力する。(USI形式ではない) デバッグ用。
std::ostream& operator<<(std::ostream& os, const Position& pos);

// depthに応じたZobrist Hashを得る。depthを含めてhash keyを求めたいときに用いる。
HASH_KEY DepthHash(int depth);

} // namespace YaneuraOu

#endif // of #ifndef POSITION_H_INCLUDED
