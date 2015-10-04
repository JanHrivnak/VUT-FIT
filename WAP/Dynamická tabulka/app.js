var puvodniTabulka;
var sloupcu;

/**
 * Vloží do zdrojové tabulky prvky pro ovládání øazení/vyhledávání
 */
function upravTabulku(idTable) {
    var table = document.getElementById(idTable);
    var oldHead = table.tHead;
    var oldHeadItems, maPuvHlavicku = false;
    puvodniTabulka = table;

    if (oldHead) {
        oldHeadItems = oldHead.rows[0].cells;
        sloupcu = oldHeadItems.length;
        maPuvHlavicku = true;
    } else { //tabulka nema hlavicku
        var tableBody = table.tBodies[0];
        if (!tableBody) {
            return true; //tabulka nema zadny obsah
        }
        var firstLine = tableBody.rows[0];
        if (!firstLine) {
            return true; //tabulka nema zadny obsah
        }
        sloupcu = firstLine.cells.length;
        oldHead = tableBody;
    }
    //vytvoøíme novou hlavièku s filtrovacími prvky
    var newHead = document.createElement('thead');
    newHead.setAttribute('id', 'sortableHead');
    for (var i = 0; i < sloupcu; i++) {
        //zjištìní typu sloupce, tj. jak máme jeho hodnoty øadit
        if (maPuvHlavicku) {
            var item = oldHeadItems[i];
            var itemType = item.getAttribute('data-type');
            if (!itemType) {
                itemType = 'string';
            }
        } else {
            itemType = 'string';
        }
        var newTd = document.createElement('td');
        newTd.innerHTML = '<div onclick="mySort(' + i + ', \'' + itemType + '\', 1);" class="sort sortAsc" title="seradit DESC jako ' + itemType + '">&darr;</div> ';
        newTd.innerHTML += ' <div onclick="mySort(' + i + ', \'' + itemType + '\', 0);" class="sort sortDesc" title="seradit ASC jako ' + itemType + '">&uarr;</div>';
        newTd.innerHTML += '<input name="cell" class="cell" id="cell_' + i + '" onkeyup="inputChange();" />';
        newHead.appendChild(newTd);
    }
    table.insertBefore(newHead, oldHead);

//    pr('Celkem ma tabulka: ' + sloupcu + ' sloupcu');
}

/**
 * Vyvolá se po každé zmìnì v jakémkoliv ovládacím inputu na stránce
 * Skrje ty øádky tabulky, které neodpovídají filtru
 */
function inputChange() {
    var body = puvodniTabulka.tBodies[0];
    var radky = body.rows;
    for (var i = 0; i < radky.length; i++) {
        var hodnoty = radky.item(i);
        var lineOk = true;
        for (var j = 0; j < sloupcu; j++) {
            var thisInput = document.getElementsByClassName('cell')[j].value;
            if (thisInput !== "") { //máme v tomto sloupci nìco filtrovat?
                var actText = hodnoty.getElementsByTagName('td').item(j).innerHTML;
                if (actText.indexOf(thisInput) == -1) { //kontrola zda øetìzec obsahuje daný podøetìzec
                    lineOk = false; //podøetìzec nenalezen
                }
            }
        }
        if (lineOk) {
            hodnoty.style.display = "";
        } else {
            hodnoty.style.display = "none";
        }
    }

}

/**
 * Seøadí tabulku podle daného sloupce. 
 * Parametry jsou Poøadí sloupce (èíslováno od 0), typ jak chceme øadit (int, string) a smìr øazení (0=ASC, 1=DESC)
 */
function mySort(id, type, smer) {
    var body = puvodniTabulka.tBodies[0];
    var radky = Array.prototype.slice.call(body.rows, 0);
    var otoc = smer === 0 ? 1 : -1; //rozedìlení ASCending/DESCending

    if (type === "int") {
        radky = radky.sort(
                function(item1, item2) {
                    if (parseInt(item1.cells[id].textContent.trim()) > parseInt(item2.cells[id].textContent.trim())) {
                        return 1 * otoc;
                    } else {
                        return -1 * otoc;
                    }
                }
        );
    } else { //øadit jako text
        radky = radky.sort(
                function(item1, item2) {
                    return (item1.cells[id].textContent.trim().localeCompare(item2.cells[id].textContent.trim())) * otoc;
                }
        );
    }
    for (var i = 0; i < radky.length; ++i) {
        body.appendChild(radky[i]);
    }
}

/**
 * Pomocná funkce pro výpis do JS konzole
 */
function pr(text) {
    window.console && console.log(text);
}

window.onload = function() {
    upravTabulku('zpracuj');
};