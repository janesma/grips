// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#ifndef SUBSCRIBER_GFSUBSCRIBER_H_
#define SUBSCRIBER_GFSUBSCRIBER_H_

#include <QObject>
#include <QString>
#include <QQmlListProperty>

#include <map>
#include <vector>

#include "remote/gfmetric.h"
#include "remote/gfisubscriber.h"
#include "os/gftraits.h"
#include "os/gfmutex.h"

namespace Grafips {

class GraphSet;

// one instance per GraphViewRenderer.  Instantiated in the GraphView,
// handles metric data events and distributes them into GraphSets that
// can be plotted by the renderer.
class GraphSetSubscriber : public QObject, public SubscriberInterface,
                           NoCopy, NoAssign, NoMove {
  Q_OBJECT
 public:
  GraphSetSubscriber();
  ~GraphSetSubscriber();
  void AddSet(int id, GraphSet *);
  void RemoveSet(int id);
  void Clear(int id);
  void OnMetric(const DataSet &d);
  void OnDescriptions(const MetricDescriptionSet &descriptions);
  void GetIDs(std::vector<int> *ids) const;

 private:
  std::map<int, GraphSet *> m_dataSets;
  mutable Mutex m_protect;
  MetricDescriptionSet m_descriptions;
};
}  // namespace Grafips
#endif  // SUBSCRIBER_GFSUBSCRIBER_H_
