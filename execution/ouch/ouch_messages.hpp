#pragma once

#include "orderbook/software/common/types.hpp"

#include <array>
#include <cstdint>

namespace ouch
{

using UserRefNum = std::uint32_t;

using Symbol =
    std::array<char, 8>;

using ClOrdId =
    std::array<char, 14>;

enum class TimeInForce : char
{
    Day = '0',
    ImmediateOrCancel = '3',
    ExtendedHours = '5',
    GoodTillTime = '6',
    AfterHours = 'E'
};

enum class Display : char
{
    Visible = 'Y',
    Hidden = 'N',
    Attributable = 'A',
    Conformant = 'Z'
};

enum class Capacity : char
{
    Agency = 'A',
    Principal = 'P',
    Riskless = 'R',
    Other = 'O'
};

enum class IsoEligibility : char
{
    Eligible = 'Y',
    NotEligible = 'N'
};

enum class CrossType : char
{
    ContinuousMarket = 'N',
    OpeningCross = 'O',
    ClosingCross = 'C',
    HaltOrIpo = 'H',
    Supplemental = 'S',
    Retail = 'R',
    ExtendedLife = 'E',
    AfterHoursClose = 'A'
};

struct EnterOrder
{
    UserRefNum userRefNum{};

    Side side{Side::Buy};

    Quantity quantity{};

    Symbol symbol{};

    std::uint64_t price{};

    TimeInForce timeInForce{
        TimeInForce::Day};

    Display display{
        Display::Visible};

    Capacity capacity{
        Capacity::Agency};

    IsoEligibility isoEligibility{
        IsoEligibility::NotEligible};

    CrossType crossType{
        CrossType::ContinuousMarket};

    ClOrdId clOrdId{};

    std::uint16_t appendageLength{};
};

enum class OrderState : char
{
    Live = 'L',
    Dead = 'D'
};

struct Accepted
{
    std::uint64_t timestamp{};

    UserRefNum userRefNum{};

    Side side{Side::Buy};

    Quantity quantity{};

    Symbol symbol{};

    std::uint64_t price{};

    TimeInForce timeInForce{
        TimeInForce::Day};

    Display display{
        Display::Visible};

    OrderId orderReferenceNumber{};

    Capacity capacity{
        Capacity::Agency};

    IsoEligibility isoEligibility{
        IsoEligibility::NotEligible};

    CrossType crossType{
        CrossType::ContinuousMarket};

    OrderState orderState{
        OrderState::Live};

    ClOrdId clOrdId{};

    std::uint16_t appendageLength{};
};


enum class RejectReason : std::uint16_t
{
    Unknown = 0
};

struct Rejected
{
    std::uint64_t timestamp{};

    UserRefNum userRefNum{};

    RejectReason reason{
        RejectReason::Unknown};

    ClOrdId clOrdId{};

    std::uint16_t appendageLength{};
};


struct Executed
{
    std::uint64_t timestamp{};

    UserRefNum userRefNum{};

    Quantity quantity{};

    std::uint64_t price{};

    char liquidityFlag{};

    std::uint64_t matchNumber{};

    std::uint16_t appendageLength{};
};

struct Canceled
{
    std::uint64_t timestamp{};

    UserRefNum userRefNum{};

    Quantity quantity{};

    char reason{};

    std::uint16_t appendageLength{};
};

} // namespace ouch
