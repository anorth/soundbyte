function button(label, colour, callback) {
  return $c('button', {className:colour+'-button',onclick:callback}, [$t(label)]);
}

function format(amount) {
  // todo
  return amount.toFixed(2);
}

function show(elem) {
  elem.style.display = '';
}
function hide(elem) {
  elem.style.display = 'none';
}

function val(input) {
  return input.value - 0;
}

window.onload = function() {
  function getAmount() {
    var total = 0;
    for (var p = people.childNodes[0]; p != addPerson; p = p.nextSibling) {
      total += val(p.paid);
    }
    return total;
  }
  function getExplicitTotal() {
    var total = 0;
    for (var p = people.childNodes[0]; p != addPerson; p = p.nextSibling) {
      total += val(p.explicit);
    }
    return total;
  }
  function adjustRem() {
    var amount = getAmount();
    var remainder = amount - getExplicitTotal();
    $('group-amount').value = format(amount);
    if (remainder > 0) {
      rem.className = 'remaining';
      remLabel.innerText = 'Remaining unshared: ';
      remAmount.innerText = format(remainder);
      show(splitOptions);
    } else if (remainder < 0) {
      rem.className = 'remaining error';
      remLabel.innerText = 'Share over by: ';
      remAmount.innerText = format(-remainder);
      hide(splitOptions);
    }
    
    if (remainder == 0) {
      hide(remainderControls);
      show(addPerson);
      hide(keepEditing);
      show(finishControls);
    } else {
      show(remainderControls);
      show(addPerson);
      hide(finishControls);
    }
  }
  function splitEven() {
    split(true);
  }
  function splitRatio() {
    split(false);
  }
  function split(even) {
    people.className = 'implicit';

    hide(remainderControls);
    hide(addPerson);
    show(keepEditing);
    show(finishControls);

    // hack
    var num = people.childNodes.length - 1;
    var explicitTotal = getExplicitTotal();
    var amount = getAmount();
    var remainder = amount - explicitTotal;
    if (!explicitTotal) {
      // degenerate case:
      even = true;
    }
    for (var n = people.childNodes[0]; n != addPerson; n = n.nextSibling) {
      n.result = val(n.explicit) + (even
          ? 1/num * remainder
          : val(n.explicit) / explicitTotal * remainder);
      n.implicit.innerText = format(n.result);
    }
  }
  function undoSplit() {
    people.className = 'explicit';
    adjustRem();
  }

  function newPerson() {
    function handleKey(event) {
      if (event.keyCode == 13) {
        newPerson();
        event.preventDefault();
      } else if (event.keyCode == 8 && this.value == ''
          && this == this.parentElement.name) {
        var p = this.parentElement.previousSibling;
        close.onclick();
        if (p) {
          p.name.focus();
        }
        event.preventDefault();
      }
    };
    var name, explicit, implicit, paid, close;
    var p = $c('div', {className:'person'}, [
        name = $c('input', {className:'name-width',
          onkeydown:handleKey, onkeypress:handleKey}),
        $t(' $'),
        explicit = $c('input', {className:'explicit', 
          onkeydown:handleKey, onkeypress:handleKey,
          onchange:adjustRem, onkeyup:adjustRem}),
        implicit = $c('label', {className:'implicit'}),
        $t(' $'),
        paid = $c('input', {className:'paid',
          onkeydown:handleKey, onkeypress:handleKey,
          onchange:adjustRem, onkeyup:adjustRem}),
        close = button('X', 'flat', function(){$rm(p);adjustRem();})
        ]);
    p.name = name;
    p.explicit = explicit;
    p.implicit = implicit;
    p.paid = paid;
    $ib(addPerson, p);
    name.focus();
  }

  var people = $('group-people');
  var addPerson = button('+ Add Person', 'green', newPerson);
  addPerson.className += ' name-width';
  $ac(people, addPerson);

  var c = $('group-options');
  var remLabel, remAmount;
  var rem = $c('span', {}, [
    remLabel = $c('label'),
    $t('$'),
    remAmount = $c('label', {className:'money'})
    ]);
  var remainderControls, splitOptions;
  $ac(c, remainderControls = $c('div', {}, [
      rem,
      splitOptions = $c('span', {}, [
          button('Split evenly', 'blue', splitEven),
          button('Split in proportion', 'blue', splitRatio)])
      ]));
  var finishControls, keepEditing;
  $ac(c, finishControls = $c('div', {}, [
      keepEditing = button('Change', 'blue', undoSplit),
      button('Done', 'green')
      ]));

  newPerson();
  undoSplit();
};

