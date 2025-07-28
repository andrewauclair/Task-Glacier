package tray;

import data.Task;
import data.TaskState;
import net.byteseek.swing.treetable.TreeUtils;
import taskglacier.MainFrame;
import tree.TaskTreeTable;

import javax.swing.*;
import java.awt.*;

class Search extends JPanel {
    private final MainFrame mainFrame;

    private TaskTreeTable newTable;

    public Search(MainFrame mainFrame) {
        this.mainFrame = mainFrame;

        Task rootObject = new Task(0, 0, "");
        newTable = new TaskTreeTable(mainFrame, rootObject, 0, false);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(newTable), gbc);
    }

    public void setSearchText(final String search) {
        newTable.setNodeFilter(treeNode -> {
            Task obj = TreeUtils.getUserObject(treeNode);
            boolean includeFinish = search.startsWith("finish: ");
            String text = search;

            if (includeFinish) {
                text = text.substring("finish: ".length());
                return !childrenHaveMatch(obj, text);
            }
            return obj.state == TaskState.FINISHED || !childrenHaveMatch(obj, text);
        });
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        return obj.name.toLowerCase().contains(text.toLowerCase());
    }
}
