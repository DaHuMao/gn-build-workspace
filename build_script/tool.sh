function RunAndLog(){
  $1 || { echo faild run: $1 && exit 1; }
  echo =========== success run $1 =========================
}

function ListeningRunProgress(){
  SEC=0
  let print_gap_time=3
  while [ $i -le 15 ]
  do
    sleep 3
    let i=$i+$print_gap_time
    ans="$(( SEC / 3600 )):$(( (SEC % 3600) / 60 )):$(( (SEC % 3600) % 60 ))"
    echo [$ans]
  done
}
