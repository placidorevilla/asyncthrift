package com.tuenti.async;

import com.stumbleupon.async.Callback;

public class GenericCallback implements Callback<Object, Object> {
  native public Object call(Object arg);
  long native_object;
}

