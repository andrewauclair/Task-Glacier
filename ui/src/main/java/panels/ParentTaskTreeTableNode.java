package panels;

import data.Task;
import org.jdesktop.swingx.treetable.TreeTableNode;

public class ParentTaskTreeTableNode extends TaskTreeTableNode {
    public ParentTaskTreeTableNode() {
        super(null, null);
    }

    public ParentTaskTreeTableNode(Task task) {
        super(null, task);
    }
}
