/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package top.niunaijun.blackbox.core;
public interface IBActivityThread extends android.os.IInterface
{
  /** Default implementation for IBActivityThread. */
  public static class Default implements top.niunaijun.blackbox.core.IBActivityThread
  {
    @Override public android.os.IBinder getActivityThread() throws android.os.RemoteException
    {
      return null;
    }
    @Override public void bindApplication() throws android.os.RemoteException
    {
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements top.niunaijun.blackbox.core.IBActivityThread
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an top.niunaijun.blackbox.core.IBActivityThread interface,
     * generating a proxy if needed.
     */
    public static top.niunaijun.blackbox.core.IBActivityThread asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof top.niunaijun.blackbox.core.IBActivityThread))) {
        return ((top.niunaijun.blackbox.core.IBActivityThread)iin);
      }
      return new top.niunaijun.blackbox.core.IBActivityThread.Stub.Proxy(obj);
    }
    @Override public android.os.IBinder asBinder()
    {
      return this;
    }
    @Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
    {
      java.lang.String descriptor = DESCRIPTOR;
      if (code >= android.os.IBinder.FIRST_CALL_TRANSACTION && code <= android.os.IBinder.LAST_CALL_TRANSACTION) {
        data.enforceInterface(descriptor);
      }
      switch (code)
      {
        case INTERFACE_TRANSACTION:
        {
          reply.writeString(descriptor);
          return true;
        }
      }
      switch (code)
      {
        case TRANSACTION_getActivityThread:
        {
          android.os.IBinder _result = this.getActivityThread();
          reply.writeNoException();
          reply.writeStrongBinder(_result);
          break;
        }
        case TRANSACTION_bindApplication:
        {
          this.bindApplication();
          reply.writeNoException();
          break;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
      return true;
    }
    private static class Proxy implements top.niunaijun.blackbox.core.IBActivityThread
    {
      private android.os.IBinder mRemote;
      Proxy(android.os.IBinder remote)
      {
        mRemote = remote;
      }
      @Override public android.os.IBinder asBinder()
      {
        return mRemote;
      }
      public java.lang.String getInterfaceDescriptor()
      {
        return DESCRIPTOR;
      }
      @Override public android.os.IBinder getActivityThread() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        android.os.IBinder _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_getActivityThread, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readStrongBinder();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public void bindApplication() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_bindApplication, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
    }
    static final int TRANSACTION_getActivityThread = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    static final int TRANSACTION_bindApplication = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
  }
  public static final java.lang.String DESCRIPTOR = "top.niunaijun.blackbox.core.IBActivityThread";
  public android.os.IBinder getActivityThread() throws android.os.RemoteException;
  public void bindApplication() throws android.os.RemoteException;
}
