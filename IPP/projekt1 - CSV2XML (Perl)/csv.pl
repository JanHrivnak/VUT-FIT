#!/usr/bin/perl

#CSV:xhrivn01
use Getopt::Long qw(:config gnu_compat);
Getopt::Long::Configure("no_ignore_case");
use utf8;
use locale;
use encoding 'utf-8';
#spoustet s :    export LC_ALL=cs_CZ.UTF-8;

use Text::CSV;
use XML::Writer;
use IO::File;
use integer;

#nevypisovat warningy (napr unknown option u getoptions)
BEGIN { $SIG{'__WARN__'} = sub { }}

##### ZPRACOVANI PARAMETRU ###
my $vysl=GetOptions(
    "help" => \$help,
    "input:s" => \@input,
    "output:s" => \@output,
    "n+" => \$argv_n,
    "r:s" => \@root,
    "s:s" => \@separator,
    "h+" => \$argv_h,
    "l:s" => \@line,
    "i+" => \$argv_i,
    "start:i" => \@start,
    "e|error-recovery+" => \$argv_e,
    "missing-value:s" => \@miss_val,
    "all-columns+" => \$all_columns,
    "padding+" => \$padding       #rozsireni PAD
);

#Byl zadan neznamy parametr
if ($vysl != 1){
  chyba("Chybne parametry",1);
}

#zjistime si delku vraceneho pole pro kazdy parametr
my $len_input = @input;
my $len_output = @output;
my $len_root = @root;
my $len_separator = @separator;
my $len_line = @line;
my $len_start = @start;
my $len_miss_val = @miss_val;

#Pokud je delka 1 (spravne zadany parametr) tak si jeho hodnotu ulozime do promenne se kterou se pak pracuje dale ve skriptu
my $input = @input[0] if($len_input==1);
my $output = @output[0] if($len_output==1);
my $root = @root[0] if($len_root==1);
my $separator = @separator[0] if($len_separator==1);
my $line = @line[0] if($len_line==1);
my $start = @start[0] if($len_start==1);
my $miss_val = @miss_val[0] if($len_miss_val==1);


##OSETRENI KOMBINACI PARAMETRU#
if($help){ # help musi samostatne
  if(!defined $input && !defined $output && !defined $argv_n && !defined $argv_h && !defined $argv_i && !defined $argv_e && !defined $root && !defined $separator && !defined $line && !defined $start && !defined $miss_val && !defined $all_columns && !defined $padding){
    tiskni_napovedu();
  }else{
    chyba("Chybna kombinace parametru",1);  # help nebyl zadan samostatne
  }
  #osetreni nepovolenych kombinaci parametru
}elsif(($argv_i && !$line) || ($start && !$argv_i) || ($miss_val && !$argv_e) || ($all_columns && !$argv_e)){
  chyba("Chybna kombinace parametru",1);
}elsif(($argv_n>1) || ($argv_h>1) || ($argv_i>1) || ($argv_e>1) || ($all_columns>1) || ($padding>1) || ($len_input>1) || ($len_output>1) || ($len_root>1) || ($len_separator>1) || ($len_line>1) || ($len_start>1) || ($len_miss_val>1)){
  chyba("Nektery parametr je zadan vicekrat",1);
}

# Osetreni tagu obalujiciho kazdy radek vystupu
if(!defined $line){
  $line='row';
}else{
  utf8::decode($line);
  if (($line =~ m/^[^\w:_].*$/)
      || ($line =~ m/^[0-9].*$/)  # \w nam projde krome znaku (vcetne diakritiky) take cislo, tak ho museme osetrit zvlast
      || ($line =~ m/[^\w:_\.\-]/)){
   #print STDERR $line."\n";
   chyba("Chybny nazev line parametru",30);
  }

}

# osetreni validity root elementu
if(defined $root){
  utf8::decode($root);
  if (($root =~ m/^[^\w:_].*$/)
     || ($root =~ m/^[0-9].*$/)
     || ($root =~ m/[^\w:_.-]/)){
    #print $root;
    chyba("Chybny nazev root parametru", 30);
  }
}

#nastaveni vychozich hodnot
if(!defined $start){
  if(!(start eq 0)){
    $start=1;
  }
}elsif($start<0){
  chyba("Parametr start nemuze byt zaporne cislo",1);
}

if(!defined $separator){
  $separator=",";
}else{
  #print $separator;
  $separator =~ s/TAB/\t/g;
  if (length($separator)>1){  #jako separator je zadan viceznakovy retezec
    chyba("Chybne zadany parametr -s",1);
  }
}
#print $separator;

if(defined $miss_val){
  utf8::decode($miss_val);
  $miss_val=uprav_entity($miss_val);
}




############## OTEVIRANI SOUBORU      #######################
$csv = Text::CSV->new ({binary => 1, eol=> $/, sep_char => $separator});

if (defined $input){
  open ($CSV, "<", $input) or chyba("Nepodarilo se otevrit vstupni soubor", 2);
}else{
  open ($CSV, "<-") or chyba("Nepodarilo se otevrit pristup k STDIN", 2);
}

my $pocet_radku=0;

### PADDING
# Musime projit cely vstup dvakrat. jednou zde abychom spocitali pocet radku vstupu at vime na kolik mist mame zarovnavat index
if($padding && $argv_i){

  if ($input){
    while (my $row = $csv->getline ($CSV)) {
       $pocet_radku++;
    }
    close $CSV;
    if($argv_h){$pocet_radku--;}

    open ($CSV, "<", $input) or chyba("Nepodarilo se otevrit vstupni soubor",2);
  }else{
    #nelze prochazet vstup ze stdin vicekrat a nejpravdepodobnejsi se jevi zarovnani na 2 mista
    $pocet_radku=20;
  }
  #print 'pocet radku v souboru: '.$pocet_radku."\n";
}
### end PADDING

# nastaveni vystupniho souboru

my $XML;
if (defined $output){
  $XML=new IO::File(">$output") or chyba("nepodarilo se otevrit vystupni soubor",3);
}else{
  $XML=<STDOUT>;
}

#my $XML= (defined $output) ? new IO::File(">$output"): <STDOUT>;

my $writer = new XML::Writer(OUTPUT => $XML, DATA_MODE => 1, UNSAFE => 1, DATA_INDENT => 3, ENCODING => 'utf-8');

#tisk UTF-8 hlavicky a root tagu
$writer->xmlDecl("UTF-8") if(!$argv_n);
$writer->startTag("$root") if($root);

my $cislo_radku=$start;
my $eof=0;
my $hlavicka=1; # v prvnim pruchodu musime vytvorit pole s nazvy sloupcu (hlavicek)
my $data=0; # slouzi k osetreni kdy je zadano -h ale ve vstupu je jenom jeden radek
my $pocet_sloupcu_v_hlavicce=0;

     ################## WHILE ##################################
     
while (my $row = $csv->getline($CSV)) {

  if($csv->eof()==1){ #jsme jiz na konci souboru (na poslednim radku je \r\n )
     $eof=1;
     break;
  }
  my @sloupce = @$row;  # nacteme si cely radek rozdelene do pole o jednotlivych sloupcich
  my $pocet_sloupcu=@sloupce;
############## OSETRENI HLAVICKY      #######################
   if ($hlavicka == 1){
      $hlavicka=0;
      $pocet_sloupcu_v_hlavicce=$pocet_sloupcu;
     #print "v hlavicce je ".$pocet_sloupcu." sloupcu\n";
     if($argv_h){
        my $i=0;
        my $nazev_sloupce;
        foreach(@sloupce){
          $nazev_sloupce=$sloupce[$i];
          utf8::decode($nazev_sloupce);
          $nazev_sloupce =~ s/[^\w:_.]/-/g ;
          if (($nazev_sloupce =~ m/^[\d-].*$/)){
            #print $nazev_sloupce;
            chyba("Chybny nazev sloupce",31);
          }
          $nazvy_sloupcu[$i]=$nazev_sloupce;
          $i++;
        }
        next; #prvni radek urcoval hlavicku, dale ho nezpracovavame
     }else{
        my $misto_v_poli=0;
        for my $i (1 .. $pocet_sloupcu){
          if($padding){                   #rozsireni PAD
             my $hodnota=prepocet($i, $pocet_sloupcu);
             $nazev_sloupce='col'.$hodnota;
          }else{
            $nazev_sloupce='col'.$i;
          }
          $nazvy_sloupcu[$misto_v_poli]=$nazev_sloupce;
          $i++;
          $misto_v_poli++;
        }
     }
   }  #prvni radek csv ?
   else{$data=1;} #kvůli osetreni zda nebyla zadana jenom hlavicka a nic vic
###################################### ZACATEK VYPISU RADKU CSV      ############################################
    if($argv_i){
      if($padding){
        $writer->startTag($line, 'index' => prepocet($cislo_radku, $pocet_radku+$start));
      }else{
        $writer->startTag($line, 'index' => $cislo_radku);
      }
    }else{
      $writer->startTag($line);
    }
    
   my $i=0;
   my $pocet_vypsanych=0;
   my $id_sloupce_navic=$pocet_sloupcu_v_hlavicce+1;

   foreach(@sloupce){
    if($pocet_vypsanych>=$pocet_sloupcu_v_hlavicce){   #je zde vice sloupcu nez v hlavicce
      if($argv_e){
        if($all_columns){ #vypis i zbyvajici sloupce
          if($argv_h){
            #projit radek az do konce a spocitat prvky a podle toho upravit nuly na zacatku
             my $pocet_navic = @sloupce;
             $pocet_navic=$pocet_navic-$pocet_sloupcu_v_hlavicce;
            if($padding){
              $nazev_sloupce='col'.(prepocet($id_sloupce_navic, $pocet_sloupcu));
            }else{
              $nazev_sloupce='col'.$id_sloupce_navic;
            }
            $id_sloupce_navic++;
          }else{
            if($padding){
              $nazev_sloupce='col'.(prepocet(($pocet_vypsanych+1), $pocet_sloupcu));
            }else{
              $nazev_sloupce='col'.($pocet_vypsanych+1);
            }
          }
          $writer->startTag($nazev_sloupce);
          my $text=$sloupce[$i];
          utf8::decode($text);
          $text=uprav_entity($text);
          $writer->raw($text);
          $writer->endTag($nazev_sloupce);
          $pocet_vypsanych++;
          $i++;
          next;
        }else{
          last;
        }
      }else{
        chyba("Jeden radek obsahuje prilis mnoho sloupcu",32);
      }
    }
    if ($pocet_sloupcu > $pocet_sloupcu_v_hlavicce && $padding && !$argv_h){ # musime zkontrolovat a pripadne upravit zarovnani nazvu sloupcu podle padding
          my $hodnota=prepocet($i, $pocet_sloupcu);
          $nazev_sloupce='col'.$hodnota;
          $writer->startTag($hodnota);
    }else{
      $writer->startTag($nazvy_sloupcu[$i]);
    }
    
    my $text=$sloupce[$i];
    utf8::decode($text);
    $text=uprav_entity($text);
    $writer->raw($text);
    if ($pocet_sloupcu > $pocet_sloupcu_v_hlavicce && $padding && !$argv_h){ # musime zkontrolovat a pripadne upravit zarovnani nazvu sloupcu podle padding
        my $hodnota=prepocet($i, $pocet_sloupcu);
        $nazev_sloupce='col'.$hodnota;
        $writer->endTag($hodnota);
    }else{
      $writer->endTag($nazvy_sloupcu[$i]);
    }
    $i++;
    $pocet_vypsanych++;
   } # foreach pres vsechny sloupce
   
   if($pocet_vypsanych<$pocet_sloupcu_v_hlavicce){
    #je zde mene sloupcu nez v hlavicce
      if(!$argv_e){
        chyba("Jeden radek obsahuje maly pocet sloupcu",32);
      }else{
        for my $i ($pocet_vypsanych+1 .. $pocet_sloupcu_v_hlavicce){
          $writer->startTag($nazvy_sloupcu[$i-1]);
          $writer->raw($miss_val) if($miss_val);
          $writer->endTag($nazvy_sloupcu[$i-1]);
        }
      }
   }
   $writer->endTag("$line");
   $cislo_radku++;
}#line in csv

#osetreni kdyz je na konci souboru prazdny radek (nebo je tam jediny prazdny radek->prazdny soubor)
if($eof != 1){
  if($hlavicka==1){    #prazdny prvni radek
    if ($argv_h){
      chyba("Prazdny soubor, ale ma zde byt hlavicka",31);
    }else{
      if(defined $argv_i){
        $writer->startTag($line, 'index' => $cislo_radku);
      }else{  #prazdny soubor. Vypiseme jeden prazdny element
        $writer->startTag($line);
      }
      $writer->startTag("col1");
      $writer->endTag("col1");
      $writer->endTag($line);
      $writer->endTag("$root") if($root);
      $writer->end();
      exit(0);
    }
  }
  if(defined $argv_e){  #osetreni CRLF na konci souboru
    if(defined $argv_i){
        $writer->startTag($line, 'index' => $cislo_radku);
      }else{
        $writer->startTag($line);
      }
    for my $i (0 .. $pocet_sloupcu_v_hlavicce-1){
      $writer->startTag($nazvy_sloupcu[$i]);
      if($i!=0){ $writer->raw($miss_val) if($miss_val);} #prazdny radek ma pro nas obsah ""CRFL proto prvni sloupec nema obsah miss_val
      $writer->endTag($nazvy_sloupcu[$i]);
    }
    $writer->endTag($line);
  }else{
    chyba("Spatne ukonceni vstupniho souboru",4);
  }
}

if(defined $argv_h && $data==0){
  chyba("Zadana pouze hlavicka, data se nemaji kde vyuzit",31);
}

  $writer->endTag("$root") if($root);
  $writer->end();

close $CSV if(defined $input);
$XML->close() if(defined $output);
exit;

#pomocna funkce ktera upravuje cislo podle rozsireni padding
#dostava jako parametry prevadene cislo a maximalni hodnotu jake cislo v tomto mistte muza mit a podle ktere se tedy musi zarovnat
sub prepocet{
  my ($hodnota, $max) = @_;
  my $delka_cisla=delka_cisla($hodnota);
  my $delka_max=delka_cisla($max);
  my $dopln=$delka_max-$delka_cisla;
  
  my $vysledek=$hodnota;
  while($dopln>0){
    $vysledek='0'.$vysledek;
    $dopln--;
  }
  return $vysledek;
}

#fce vraci pocet znaku cisla
sub delka_cisla{
  my ($cislo) = @_;
  my $delka=0;
  do{ $cislo=$cislo/10;
      $delka++;
  }while ($cislo>0);
  return $delka;
}

#fce prevadi nepovolene entity na jejich zapis s &
sub uprav_entity{
  my ($text)=@_;
  $text =~ s/[&]/&amp;/g ;
  $text =~ s/[\"]/&quot;/g ;
  $text =~ s/[<]/&lt;/g ;
  $text =~ s/[>]/&gt;/g ;
 return $text;
}

#funkce na osetreni chyb
sub chyba{
   my ($text, $param)=@_;
   print STDERR $text."\n";
   exit($param)
}

#funkce pro tisk napovedy k programu
sub tiskni_napovedu{
print "Napoveda:";
    print "\nIPP: proj1 CSV2XML";
    print "\n2011/2012L";
    print "\nAutor: xhrivn01";
    print "\n\t Použití:\n";
    print "\n\t --help - vypise tuto napovedu";
    print "\n\t --input=filename - vstupni soubor";
    print "\n\t --output=filename - vystupni soubor";
    print "\n\t -n - negenerovat XML hlavicku";
    print "\n\t -r=root-element - element obalujici vysledek";
    print "\n\t -s=separator - oddelovac zaznamu";
    print "\n\t -h - jako nazvy sloupcu pouzije prvni radek";
    print "\n\t -l=line-element - obaluje každy radek vystupnich dat";
    print "\n\t -i - vlozi index do kazdeho radku";
    print "\n\t --start=n - indexovat se zacne od n";
    print "\n\t -e, --error-recovery - pri spatnem poctu sloupcu neskonci chybou";
    print "\n\t --missing-value=val - chybejici hodnota se doplni touto";
    print "\n\t --all-columns - vypise vsechny sloupce";
    print "\n\t --padding -rozsireni, zarovna indexy na stejny pocet mist\n";
    exit(0);
}