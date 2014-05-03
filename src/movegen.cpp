/*
  Challenger, a UCI chess playing engine derived from Stockfish
  
  

  Challenger is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Challenger is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "movegen.h"
#include "position.h"
#include "notation.h"

/// Simple macro to wrap a very common while loop, no facny, no flexibility,
/// hardcoded names 'mlist' and 'from'.
#define SERIALIZE(b) while (b) (mlist++)->move = make_move(from, pop_lsb(&b))

/// Version used for pawns, where the 'from' square is given as a delta from the 'to' square
#define SERIALIZE_PAWNS(b, d) while (b) { Square to = pop_lsb(&b); \
                                         (mlist++)->move = make_move(to - (d), to); }
namespace {

 
  template<Color Us, GenType Type>
  ExtMove* generate_pawn_moves(const Position& pos, ExtMove* mlist,
                               Bitboard target, const CheckInfo* ci) {

    // Compute our parametrized parameters at compile time, named according to
    // the point of view of white side.
    const Color    Them     = (Us == WHITE ? BLACK    : WHITE);
    const Square   Up       = (Us == WHITE ? DELTA_N  : DELTA_S);
	const Square   Right    = (Us == WHITE ? DELTA_E : DELTA_W);
    const Square   Left     = (Us == WHITE ? DELTA_W : DELTA_E);
	const Bitboard MaskBB   =  PawnMask[Us];

    Bitboard b1, b2, b3, dc1, dc2, dc3, emptySquares;

    Bitboard enemies = (Type == EVASIONS ? pos.pieces(Them) & target:
                        Type == CAPTURES ? target : pos.pieces(Them));

	Bitboard pawns   = pos.pieces(Us, PAWN) & MaskBB;
	Bitboard passedRiver = pawns & PassedRiverBB[Us];

	//target:��Բ�ͬ��move���ͣ��в�ͬ��Ŀ��   
    if (Type != CAPTURES)
    {
        emptySquares = (Type == QUIETS || Type == QUIET_CHECKS ? target : ~pos.pieces());

		b1 = shift_bb<Up>(pawns) & emptySquares;
		b2 = shift_bb<Left>(pawns) & emptySquares;
		b3 = shift_bb<Right>(pawns) & emptySquares;

        if (Type == EVASIONS) // Consider only blocking squares
        {
            b1 &= target;
            b2 &= target;
			b3 &= target;
        }

        if (Type == QUIET_CHECKS)
        {
          
			b1 &= pos.attacks_from_pawn_nomask(ci->ksq, Them) & PassedRiverBB[Them];
			b2 &= pos.attacks_from_pawn_nomask(ci->ksq, Them) & PassedRiverBB[Them];
			b3 &= pos.attacks_from_pawn_nomask(ci->ksq, Them) & PassedRiverBB[Them];

			if (passedRiver & ci->dcCandidates)
            {
                //����ǣ�ƣ�����ǣ�ƣ�����ǣ��
				//����ǣ�ƣ��κ�λ�ö�����
				//����ǣ�ƣ�����kingͬ��ͬ��
				//����ǣ�ƣ�����kingͬ��ͬ��

				dc1 = shift_bb<Up>(passedRiver & ci->dcCandidates) & emptySquares;
                dc2 = shift_bb<Left>(passedRiver & ci->dcCandidates) & emptySquares;
				dc3 = shift_bb<Right>(passedRiver & ci->dcCandidates) & emptySquares;

                b1 |= dc1;
                b2 |= dc2;
				b3 |= dc3;
            }
			else
			{
				//�䵱�ڼ�
				dc1 = shift_bb<Up>(passedRiver) & emptySquares & ci->forbid;
				dc2 = shift_bb<Left>(passedRiver) & emptySquares & ci->forbid;
				dc3 = shift_bb<Right>(passedRiver) & emptySquares & ci->forbid;

				b1 |= dc1;
				b2 |= dc2;
				b3 |= dc3;
			}
        }

		b1 &= MaskBB;
		b2 &= MaskBB;
		b3 &= MaskBB;


        SERIALIZE_PAWNS(b1, Up);
        SERIALIZE_PAWNS(b2, Left);
		SERIALIZE_PAWNS(b3, Right);
    }

    // Standard and captures
    if (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
        
		b1 = shift_bb<Right>(pawns) & enemies;
        b2 = shift_bb<Left >(pawns) & enemies;
		b3 = shift_bb<Up >(pawns) & enemies;

		b1 &= MaskBB;
		b2 &= MaskBB;
		b3 &= MaskBB;

        SERIALIZE_PAWNS(b1, Right);
        SERIALIZE_PAWNS(b2, Left);
		SERIALIZE_PAWNS(b3, Up);
    }

    return mlist;
  }


 /* template<PieceType Pt, bool Checks> FORCE_INLINE
  ExtMove* generate_moves(const Position& pos, ExtMove* mlist, Color us,
                          Bitboard target, const CheckInfo* ci) {

    assert(Pt != KING && Pt != PAWN);

    const Square* pl = pos.list<Pt>(us);

    for (Square from = *pl; from != SQ_NONE; from = *++pl)
    {
        if (Checks)
        {
            if (    (Pt == BISHOP || Pt == ROOK || Pt == QUEEN)
                && !(PseudoAttacks[Pt][from] & target & ci->checkSq[Pt]))
                continue;

            if (unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
                continue;
        }

        Bitboard b = pos.attacks_from<Pt>(from) & target;

        if (Checks)
            b &= ci->checkSq[Pt];

        SERIALIZE(b);
    }

    return mlist;
  }*/

  template<PieceType Pt, bool Checks> FORCE_INLINE
	  ExtMove* generate_moves(const Position& pos, ExtMove* mlist, Color us,
	  Bitboard target, const CheckInfo* ci)
  {

	  assert(Pt != KING && Pt != PAWN);

	  //rook,cannon,knight,bishop��advisor

	  const Square* pl = pos.list<Pt>(us);

	  for (Square from = *pl; from != SQ_NONE; from = *++pl)
	  {
		  //Checks��ʾquiet check����˼,�й����������淽��������
		  if (Checks)
		  {
			  ////����ֱ�ӽ���
			  //if ( (Pt == ROOK || Pt == CANNON)
				 // && !(PseudoAttacks[Pt][from] & target & ci->checkSq[Pt]))
				 // continue;

			  ////Checks��ʾquiet check����˼��������������ڵ����������֮ǰ�Ѿ�����
			  //if (unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
				 // continue;

			  //�������Ӳ���ֱ�ӽ��������ǿ��������ڷǳ��ӽ���
			  //if(Pt == BISHOP || Pt == ADVISOR)
				 // continue;
		  }

		  Bitboard b;

		  if(Pt == ADVISOR )
		  {
			  b = pos.attacks_from<ADVISOR>(from, us) & target;
		  }
		  else if(Pt == BISHOP)
		  {
			  b = pos.attacks_from<BISHOP>(from, us) & target;
		  }
		  else if(Pt == CANNON)
		  {
			  b = pos.attacks_from<ROOK>(from) & target & (~pos.pieces());//�ǳ��ӣ���һ���У�targetҲҪ���Ƴ��Ӻͷǳ��ӣ����������зǳ��Ӽ���Ҳ����ȷ��
			 
			  b |= pos.attacks_from<CANNON>(from) & target & pos.pieces();//����
			 
		  }
		  else if(Pt == ROOK)
		  {
			  b = pos.attacks_from<ROOK>(from) & target;	
		  }
		  else if(Pt == KNIGHT)
		  {
			  b = pos.attacks_from<KNIGHT>(from) & target;
		  }

		  //checkSq[Pt]��ʾPt����check�Է�king�ĵط���������ֱ��check����ֱ��check��������,���������Ҫ�ر���
		  //���ڷ�ֱ��check���ӣ����Ե��ڼܣ����Ե��赲rook check����
		  //������ʹ��֮ǰ�Ѿ�������ˣ����������ٶ�
		  //����ֱ��check���ӣ�b &= ci->checkSq[Pt]�����ٶȣ���ֹÿ���ֶ�����check�ļ���
		  //���ڷ�ֱ��check����,Ҫcheck����
		  if (Checks)
		  {
			  //�����������⣬�� -*-c-----k---, c�ƶ���*���ᱻ��Ϊ��quiet check
			  if(Pt == CANNON)
			  {
				  if(unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
				  {
					  //�������ڻ���ǣ��
					  //����ǣ�ƣ�������kingͬ��ͬ��
					  //����ǣ�ƣ�������kingͬ��ͬ��
                      //��ǣ�ƣ��κ�λ�ö�����                     

					  //���from����ci.ksqͬ�л�ͬ�У����Ǳ���ǣ��
					  if(PseudoAttacks[ROOK][ci->ksq] & from)
					  {
						  //��king ͬ�л�ͬ�У��ض��Ǳ�����ǣ��
						  b &= ~PseudoAttacks[ROOK][ci->ksq];
					  }
					  //�����Ǳ���ǣ�ƣ��κ�λ�ö�����

				  }
				  else
				  {
				      //�����kingͬ��ͬ�У���ô�ǳ��ӵ��κ�λ�ö�������quiet check
                      if( !(PseudoAttacks[ROOK][ci->ksq] & from))
					  {
						  //from ����kingͬ��ͬ�У����������������
                          b &= ci->checkSq[Pt]|ci->forbid;//����Ϊ�ڼ�
					  }
					  else
					  {
						  //��kingͬ��ͬ�У����Ҳ��Ǳ�ǣ�ƣ����κ�λ�ö���������quiet check
						  b = Bitboard();
					  }
					  
				  }
			  }
			  else if(Pt == ROOK)
			  {
				  if(unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
				  {
					  //���ڻ���ǣ��
					  //��ǣ�ƣ��κ�λ�ö�����
					  //��ǣ�ƣ�������Է���kingͬ�л�ͬ��

					  if(PseudoAttacks[ROOK][ci->ksq] & from)
					  {
						  //��kingͬ��ͬ�У��ض����ڻ�ǣ��
						  b &= ~PseudoAttacks[ROOK][ci->ksq];
					  }
					  //������ǣ�ƣ��κ�λ�ö�����
				  }
				  else
				  {
					  b &= ci->checkSq[Pt];//�Ѿ��������ڼ�
				  }
			  }
			  else if(Pt == KNIGHT)
			  {
				  if(unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
				  {
					  //�������ڻ���ǣ��
					  //�κ�λ�ö���check
				  }
				  else
				  {
					  b &= ci->checkSq[Pt]|ci->forbid;//check���ڼ�
				  }
			  }
			  else if(Pt == BISHOP || Pt == ADVISOR)
			  {
				  //��������ǣ��
				  //�κ�λ�ö�����
                  if(unlikely(ci->dcCandidates) && (ci->dcCandidates & from))
				  {
				  }
				  else
				  {
					  //�������䵱�ڼ�
                      b &= ci->forbid;
				  }
			  }
		  }

		  SERIALIZE(b);
	  }

	  return mlist;
  }


  template<Color Us, GenType Type> FORCE_INLINE
  ExtMove* generate_all(const Position& pos, ExtMove* mlist, Bitboard target,
                        const CheckInfo* ci = NULL) {

    const bool Checks = Type == QUIET_CHECKS;

	Bitboard fbbtarget = (~pos.cannon_forbid_bb(Us)) & target;//���ܵ��ڼ�

    mlist = generate_pawn_moves<Us, Type>(pos, mlist, fbbtarget, ci);
    mlist = generate_moves<KNIGHT, Checks>(pos, mlist, Us, fbbtarget, ci);
    mlist = generate_moves<BISHOP, Checks>(pos, mlist, Us, fbbtarget, ci);
    mlist = generate_moves<  ROOK, Checks>(pos, mlist, Us, fbbtarget, ci);
	mlist = generate_moves<CANNON, Checks>(pos, mlist, Us, fbbtarget, ci);
	mlist = generate_moves<ADVISOR, Checks>(pos, mlist, Us, fbbtarget, ci);

    //EVASIONS ����ǰ�Ѿ�����
	//QUIET_CHECKS ?����ǰ�Ѿ�����
	if (Type != QUIET_CHECKS && Type != EVASIONS)
    {
        Square from = pos.king_square(Us);
        Bitboard b = pos.attacks_from<KING>(from, Us) & target;
        SERIALIZE(b);
    }

    return mlist;
  }


} // namespace


/// generate<CAPTURES> generates all pseudo-legal captures and queen
/// promotions. Returns a pointer to the end of the move list.
///
/// generate<QUIETS> generates all pseudo-legal non-captures and
/// underpromotions. Returns a pointer to the end of the move list.
///
/// generate<NON_EVASIONS> generates all pseudo-legal captures and
/// non-captures. Returns a pointer to the end of the move list.

template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* mlist) {

  assert(Type == CAPTURES || Type == QUIETS || Type == NON_EVASIONS);
  assert(!pos.checkers());

  Color us = pos.side_to_move();

  Bitboard target = Type == CAPTURES     ?  pos.pieces(~us)
                  : Type == QUIETS       ? ~pos.pieces()
                  : Type == NON_EVASIONS ? ~pos.pieces(us) : Bitboard();

  return us == WHITE ? generate_all<WHITE, Type>(pos, mlist, target)
                     : generate_all<BLACK, Type>(pos, mlist, target);
}

// Explicit template instantiations
template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);


/// generate<QUIET_CHECKS> generates all pseudo-legal non-captures and knight
/// underpromotions that give check. Returns a pointer to the end of the move list.
template<>
ExtMove* generate<QUIET_CHECKS>(const Position& pos, ExtMove* mlist) {

  assert(!pos.checkers());

  Color us = pos.side_to_move();
  CheckInfo ci(pos);
  Bitboard dc = ci.dcCandidates;
  Square from = pos.king_square(us);

  //��ǣ��������check,����������ǣ�Ƶ���
  //����������cCandidates������pawn��kinght��bishop,king,rook��������queen������ֱ�ӽ���
  //pawn�����Candidates,ֻ����pawn�Լ���check��queen,bishop,rook check
  //knight,�����ﶼ���Բ���check
  //bishop,�����ﶼ���Բ���check
  //rook,�����ﶼ���Բ���check
  //queen,ֻ��ֱ��check
  //king,Ҫ����Է�kingͬ�����˶�
  //���������������ǳ�����
  //�й�������������ӣ�����Ĳ�����
  //while (dc)
  //{
  //   Square from = pop_lsb(&dc);
  //   PieceType pt = type_of(pos.piece_on(from));

  //   //���Ҳ����quiet check�����洦��,������ٶ�
	 //if (pt == PAWN)
  //       continue; // Will be generated togheter with direct checks

  //   //Bitboard b = pos.attacks_from(Piece(pt), from) & ~pos.pieces();

	 //Bitboard b;

	 ////if(pt == CANNON)
	 ////{         
		//// //������ڣ����ƶ���������Է�kingͬһ���������ͬһ����ֻ���ǳ��Ӳ��ܽ���������費��
		//// //�����ڿ���������
		//// b = pos.attacks_from<ROOK>(from) & ~pos.pieces();
		//// //b &= ~PseudoAttacks[ROOK][ci.ksq];//king�����ϵ��ƶ�,���ڿ���������
	 ////}
	 ////else
	 ////{
		//// b = pos.attacks_from(pos.piece_on(from), from) & ~pos.pieces();
	 ////}

	 //if(pt == ADVISOR)
	 //{
  //      b = pos.attacks_from(pos.piece_on(from), from) & ~pos.pieces();
	 //}
	 //else if(pt == BISHOP)
	 //{
		//b = pos.attacks_from(pos.piece_on(from), from) & ~pos.pieces();
	 //}
	 //else if (pt == KING)
	 //{
		// b &= ~PseudoAttacks[ROOK][ci.ksq];//ֻ����cannonǣ�ƣ�������Է�king��һ������;�������û��king������gen king��quiet check�����Ժ���Ѹ�����ų�������
	 //}

  //   SERIALIZE(b);
  //}

  //����ֻ��kingһ����������Կ���ֱ���ж�king�ǲ���dcCandidates
  if(dc&from)
  {
        Bitboard b = pos.attacks_from(pos.piece_on(from), from) & ~pos.pieces();

		b &= ~PseudoAttacks[ROOK][ci.ksq];

        SERIALIZE(b);
  }

  return us == WHITE ? generate_all<WHITE, QUIET_CHECKS>(pos, mlist, ~pos.pieces(), &ci)
                     : generate_all<BLACK, QUIET_CHECKS>(pos, mlist, ~pos.pieces(), &ci);
}

/// generate<EVASIONS> generates all pseudo-legal check evasions when the side
/// to move is in check. Returns a pointer to the end of the move list.
template<>
ExtMove* generate<EVASIONS>(const Position& pos, ExtMove* mlist) {

  assert(pos.checkers());

  Color    us = pos.side_to_move();
  Square   ksq = pos.king_square(us), from = ksq /* For SERIALIZE */, checksq;
  Bitboard sliderAttacks;
  Bitboard knightAttacks;
  Bitboard cannonAttacks;
  Bitboard pawnAttacks;
  Bitboard b = pos.checkers();
  PieceType chkType1 = NO_PIECE_TYPE;
  PieceType chkType2 = NO_PIECE_TYPE;
  Square    chksq1, chksq2;
  int       checkersCnt = 0;

  assert(pos.checkers());

  // Find squares attacked by slider checkers, we will remove them from the king
  // evasions so to skip known illegal moves avoiding useless legality check later.
  //��������ֻ��slider check,�й��������Ͷ�
  do
  {
      checkersCnt++;
      checksq = pop_lsb(&b);

      assert(color_of(pos.piece_on(checksq)) == ~us);

	  if(checkersCnt == 1)
	  {
		  chkType1 = type_of(pos.piece_on(checksq));
		  chksq1 = checksq;
	  }
	  if(checkersCnt == 2)
	  {
		  chkType2 = type_of(pos.piece_on(checksq));
		  chksq2 = checksq;
	  }

      switch (type_of(pos.piece_on(checksq)))
      {
      case ROOK:   sliderAttacks |= PseudoAttacks[ROOK][checksq];   break;//��˼�ǣ����rook��������ô����ж��Ǳ�������
	  case KNIGHT: knightAttacks |= knight_attacks_bb(checksq,pos.occupied); break;
	  case CANNON: cannonAttacks |= cannon_control_bb(checksq,pos.occupied,pos.occupied_rl90)|cannon_supper_pin_bb(checksq,pos.occupied,pos.occupied_rl90); break;
	  case PAWN:   pawnAttacks   |= pos.attacks_from<PAWN>(checksq, ~us); break;

      default:
          break;
      }
  } while (b);

  // Generate evasions for king, capture and non capture moves
  b = pos.attacks_from<KING>(ksq,us) & ~pos.pieces(us) & ~sliderAttacks & ~knightAttacks & ~cannonAttacks & ~pawnAttacks;//�����ߵ����ֹ����ĵط�
  SERIALIZE(b);

  if (checkersCnt > 2)
      return mlist; // Double check, only a king move can save the day;�й����������3��������ֻ���ƶ���

   // Generate blocking evasions or captures of the checking piece
   //Bitboard target = between_bb(checksq, ksq) | checksq;//�й�����ϸ���
  Bitboard target;
  if(checkersCnt == 1 )
  {
	  if(chkType1 == ROOK)
	  {
		  target |= between_bb(checksq, ksq) | checksq;//�Ե�,ROOK����Ҫ��
	  }
	  else if(chkType1 == CANNON)
	  {
		  //�ų��ڼ����ڵ����,
		  Bitboard mid = between_bb(checksq,ksq);
          Bitboard battery = mid&pos.occupied;
		  Bitboard t = (battery & pos.pieces(~us, CANNON) & (~SquareBB[checksq]));
		  if(!t)
		  {
			  target |= (mid|checksq) & ( ~battery);////��,Ҫȥ���ڼܣ�λ���̴���������ȷ�ܼ�
		  }
		  else
		  {
			  target |= (between_bb(checksq, lsb(t)) | checksq) ;//����֮��
		  }

		  //����ڼ����ҷ��ӣ��������ƶ�����ڼܣ����ǲ��ܷ���target���У�
		  //���������Ҫ��������

		  //������������ڼ����ҷ��ӣ����Ӳ������Ž��������ƶ�

		  //��ֻҪ�ڼ����ҷ��ӣ�����ֻ�ƶ������
		  //�����ƶ����Ӳ������ڼ�
		  //�ԣ�

		  //��
		  //��
		  //��

		  {
			  Bitboard forbid  = pos.cannon_forbid_bb(us);
			  Square sqbattery = lsb(battery);
			  for(PieceType Pt = PAWN; Pt <= ROOK; ++Pt)
			  {
				  const Square* pl = 0;
				  if(Pt == PAWN)  pl = pos.list<PAWN>(us);
				  else if(Pt == BISHOP)  pl = pos.list<BISHOP>(us);
				  else if(Pt == ADVISOR)  pl = pos.list<ADVISOR>(us);
				  else if(Pt == CANNON)  pl = pos.list<CANNON>(us);
				  else if(Pt == KNIGHT)  pl = pos.list<KNIGHT>(us);
				  else if(Pt == ROOK)  pl = pos.list<ROOK>(us);
				  

				  for (Square from = *pl; from != SQ_NONE; from = *++pl)
				  {

					  Bitboard b;

					  if( Pt == PAWN)
					  {
                          Bitboard att = pos.attacks_from<PAWN>(from, us) & ~pos.pieces(us);
						  if(sqbattery != from)
						  {  
							  b =  att & target;
						  }//�Ի�
						  else
						  {  
							  b = att & SquareBB[checksq];//��
                              b |= att & (~mid);//��
						  }
					  }
					  else if( Pt == BISHOP)
					  {
						  Bitboard att = pos.attacks_from<BISHOP>(from, us) & ~pos.pieces(us);
						  if(sqbattery != from)
						  {
							  b =  att & target;
						  }
						  else
						  {
                              b = att & SquareBB[checksq];//��
                              b |= att & (~mid);//��
						  }
					  }
					  else if( Pt == ADVISOR)
					  {
						  Bitboard att = pos.attacks_from<ADVISOR>(from, us) & ~pos.pieces(us);
						  if(sqbattery != from)
						  {
							  b =  att & target;
						  }
						  else
						  {
							  b = att & SquareBB[checksq];//��
							  b |= att & (~mid);//��
						  }
					  }
					  else if(Pt == CANNON)
					  {
						  Bitboard natt = pos.attacks_from<ROOK>(from) & (~pos.pieces());//�ǳ���
                          Bitboard att =  pos.attacks_from<CANNON>(from) & pos.pieces(~us);
						  if(sqbattery != from)
						  {
							  b =  (natt|att) & target;//�Ի�
						  }
						  else
						  {
							  b = att & SquareBB[checksq];//��
							  b |= (att & (~mid))|(natt &(~mid));//��
							 
						  }					  
					  }
					  else if(Pt == ROOK)
					  {
						  Bitboard att =  pos.attacks_from<ROOK>(from) & ~pos.pieces(us);
						  if(sqbattery != from)
						  {
							  b =  att & target;
						  }
						  else
						  {
							  b = att & SquareBB[checksq];//��							  
							  b |= att & (~mid);//��							 
						  }
					  }
					  else if(Pt == KNIGHT)
					  {
						  Bitboard att =  pos.attacks_from<KNIGHT>(from) & ~pos.pieces(us);
						  if(sqbattery != from)
						  {
							  b =  att & target;
						  }
						  else
						  {
							  b = att & SquareBB[checksq];//��
							  b |= att & (~mid);//��
						  }
					  }
                      
					  b &= ~forbid;
					  SERIALIZE(b);
				  }
			  }//for type
		  }

		  return mlist;
	  }
	  else if(chkType1 == KNIGHT)
	  {
		  //�����Ȼ����Ե�
		  if( (ksq - checksq == 19) ||  (ksq - checksq == 11))//king��sq��S��
		     target |= SquareBB[(ksq-10)] | checksq;

		  else if( (ksq - checksq == 17) ||  (ksq - checksq == 7))
			  target |= SquareBB[(ksq-8)] | checksq;

		  else if( (ksq - checksq == -19) ||  (ksq - checksq == -11))
			  target |= SquareBB[(ksq+10)] | checksq;

		  else if( (ksq - checksq == -17) ||  (ksq - checksq == -7))
			  target |= SquareBB[(ksq+8)] | checksq;
	  }
	  else if(chkType1 == PAWN)
	  {
		  target |= SquareBB[checksq];//ֻ�ܳԵ���������������ƶ���֮����ǳԵ�
	  }
  }
  else if(checkersCnt == 2)
  {
	  //ֻ���ǳ������ڣ��ڱ������������ ����ȼ������
	  if( ((chkType1 == ROOK) && (chkType2 == KNIGHT) ) || ((chkType1 == KNIGHT) && (chkType2 == ROOK ) ) )
	  {
		  //ֻ���ƶ���
          return mlist;
	  }

	  if( ((chkType1 == ROOK) && (chkType2 == CANNON) ) || ((chkType1 == CANNON) && (chkType2 == ROOK ) ) )
	  {
		  //�ƶ�����
         if(chkType1 == ROOK)
		 {
			 target |= between_bb(chksq1, ksq) & cannon_control_bb(chksq2,pos.occupied,pos.occupied_rl90);
		 }
		 else if(chkType2 == ROOK)
		 {
			 target |= between_bb(chksq2, ksq) & cannon_control_bb(chksq1,pos.occupied,pos.occupied_rl90);
		 }
	  }

	  if( ((chkType1 == PAWN) && (chkType2 == CANNON) ) || ((chkType1 == CANNON) && (chkType2 == PAWN ) ) )
	  {
		  //�����ڽ�����ֻ���ƶ������ԳԱ�,�����ﲻ���ƶ���
		  return mlist;		  
          
	  }

	  if( ((chkType1 == KNIGHT) && (chkType2 == CANNON) ) || ((chkType1 == CANNON) && (chkType2 == KNIGHT ) ) )
	  {
		  ////ֻ���ƶ�
		  //return mlist;
		  //�����Ȼ����Ե�,���ܻ�

		  if(chkType1 == KNIGHT)
             checksq = chksq1;
		  else
		     checksq = chksq2;

		  if( (ksq - checksq == 19) ||  (ksq - checksq == 11))//king��sq��S��
		     target |= SquareBB[(ksq-10)] | checksq;

		  else if( (ksq - checksq == 17) ||  (ksq - checksq == 7))
			  target |= SquareBB[(ksq-8)] | checksq;

		  else if( (ksq - checksq == -19) ||  (ksq - checksq == -11))
			  target |= SquareBB[(ksq+10)] | checksq;

		  else if( (ksq - checksq == -17) ||  (ksq - checksq == -7))
			  target |= SquareBB[(ksq+8)] | checksq;
	  }

	  if( ((chkType1 == KNIGHT) && (chkType2 == PAWN) ) || ((chkType1 == PAWN) && (chkType2 == KNIGHT ) ) )
	  {
		  //ֻ���ƶ�
		  return mlist;
	  }

	  if( ((chkType1 == KNIGHT) && (chkType2 == KNIGHT) ) || ((chkType1 == KNIGHT) && (chkType2 == KNIGHT ) ) )
	  {

		  if( ((ksq - chksq1 == 19) &&  (ksq - chksq2 == 11)) || ((ksq - chksq2 == 19) && (ksq - chksq1 == 11)))
		     target |= SquareBB[(ksq-10)] ;

		  else if( ((ksq - chksq1 == 17) &&  (ksq - chksq2 == 7)) || ((ksq - chksq2 == 17) &&  (ksq - chksq1 == 7)))
			  target |= SquareBB[(ksq-8)] ;

		  else if( ((ksq - chksq1 == -19) &&  (ksq - chksq2 == -11)) || ((ksq - chksq2 == -19) &&  (ksq - chksq1 == -11)))
			  target |= SquareBB[(ksq+10)];

		  else if( ((ksq - chksq1 == -17) &&  (ksq - chksq2 == -7)) || ((ksq - chksq2 == -17) &&  (ksq - chksq1 == -7)))
			  target |= SquareBB[(ksq+8)];
		  else
		  {
			   return mlist;
		  }
	  }
  }
  

  return us == WHITE ? generate_all<WHITE, EVASIONS>(pos, mlist, target)
                     : generate_all<BLACK, EVASIONS>(pos, mlist, target);

}


/// generate<LEGAL> generates all the legal moves in the given position

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* mlist)
{

  ExtMove *end, *cur = mlist;
  Bitboard pinned = pos.pinned_pieces();//Ҫ���ǲ����ߵ��ڼ����ϣ�Ҫ���Ƕ���
  Square ksq = pos.king_square(pos.side_to_move());

  end = pos.checkers() ? generate<EVASIONS>(pos, mlist)
                       : generate<NON_EVASIONS>(pos, mlist);
  while (cur != end)
  {
	  //std::cout<<move_to_chinese(pos, cur->move)<<std::endl;
	  if (   (pinned || from_sq(cur->move) == ksq || (pos.checkers() & pos.pieces(~pos.side_to_move(),CANNON) ) )
          && !pos.pl_move_is_legal(cur->move, pinned))
	  {
		  cur->move = (--end)->move;
	  }
      else
	  {
		  cur++;
	  }
  }

  return end;
}

void test_move_gen( Position& pos)
{
	//Bitboard target;
	//target = ~target;

	std::cout<<"------------------"<<std::endl;

	ExtMove mlist[MAX_MOVES];
    ExtMove *cur, *last;
	cur = mlist;
	
	//last = generate_pawn_moves<WHITE, NON_EVASIONS>(pos, mlist, target, 0);
	// last = generate_moves<  ROOK, false>(pos, mlist, WHITE, target, 0);

	// last = generate_moves<  KNIGHT, false>(pos, mlist, WHITE, target, 0);
	//last = generate_moves<  CANNON, false>(pos, mlist, WHITE, target, 0);
	//last = generate_moves<  ADVISOR, false>(pos, mlist, WHITE, target, 0);
	//last = generate_moves<  BISHOP, false>(pos, mlist, WHITE, target, 0);
	///last = generate_moves<  KING, false>(pos, mlist, WHITE, target, 0);

	//last = generate<CAPTURES>(pos, mlist);
	//last = generate<QUIETS>(pos, mlist);
	//last = generate<NON_EVASIONS>(pos, mlist);
	last = generate<QUIET_CHECKS>(pos, mlist);

	std::cout<< std::endl;
	for(; cur != last; ++cur)
	{
		std::cout<< move_to_chinese(pos,cur->move).c_str();
	}
}
