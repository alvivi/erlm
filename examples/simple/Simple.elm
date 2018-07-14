port module Simple exposing (main)

{-
   To avoid "ReferenceError: _elm_lang$core$Json_Decode$int is not defined" we
   need to include JSON.Decode
-}

import Json.Decode


type Msg
    = PlusOne Int


port result : Int -> Cmd msg


port plusOne : (Int -> msg) -> Sub msg


update : Msg -> () -> ( (), Cmd Msg )
update (PlusOne x) _ =
    ( (), result (x + 1) )


main : Program Never () Msg
main =
    Platform.program
        { init = ( (), result 69 )
        , update = update
        , subscriptions = always <| plusOne PlusOne
        }
